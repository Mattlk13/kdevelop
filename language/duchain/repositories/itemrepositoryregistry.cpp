/*
   Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "itemrepositoryregistry.h"

#include <QtCore/QDir>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QCoreApplication>
#include <QtDBus/QDBusConnection>

#include <KStandardDirs>
#include <KDebug>
#include <KLocalizedString>

#include <interfaces/icore.h>
#include <shell/sessioncontroller.h>
#include <util/fileutils.h>
#include <util/shellutils.h>

#include "abstractitemrepository.h"

namespace {

//If KDevelop crashed this many times consicutively, clean up the repository
const int crashesBeforeCleanup = 2;

void setCrashCounter(QFile& crashesFile, int count)
{
  crashesFile.close();
  crashesFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
  QDataStream writeStream(&crashesFile);
  writeStream << count;
}

QString repositoryPathForSession(KDevelop::ISession* session)
{
  QString xdgCacheDir = QProcessEnvironment::systemEnvironment().value("XDG_CACHE_HOME", QDir::homePath() + "/.cache") + "/kdevduchain";
  QString baseDir = QProcessEnvironment::systemEnvironment().value("KDEV_DUCHAIN_DIR", xdgCacheDir);
  baseDir += QString("/%1-%2").arg(qAppName()).arg(session->id().toString());
  KStandardDirs::makeDir(baseDir);
  return baseDir;
}

}

namespace KDevelop {

struct ItemRepositoryRegistryPrivate {
  ItemRepositoryRegistry* m_owner;
  bool m_shallDelete;
  QString m_path;
  QMap<AbstractItemRepository*, AbstractRepositoryManager*> m_repositories;
  QMap<QString, QAtomicInt*> m_customCounters;
  mutable QMutex m_mutex;

  ItemRepositoryRegistryPrivate(ItemRepositoryRegistry* owner) :
  m_owner(owner),
  m_mutex(QMutex::Recursive)
  {
  }

  void lockForWriting();
  void unlockForWriting();
  void deleteDataDirectory(bool recreate = true);
};

//The global item-reposity registry
ItemRepositoryRegistry* ItemRepositoryRegistry::m_self = 0;

ItemRepositoryRegistry::ItemRepositoryRegistry() :
d(new ItemRepositoryRegistryPrivate(this))
{
  Q_ASSERT(ICore::self());
  Q_ASSERT(ICore::self()->activeSession());
  QString repositoryPath = repositoryPathForSession(ICore::self()->activeSession());
  open(repositoryPath, false);
}

ItemRepositoryRegistry* ItemRepositoryRegistry::self()
{
  if(!m_self) {
    ///We intentionally leak the registry, to prevent problems in the destruction order, where
    ///the actual repositories might get deleted later than the repository registry.
    m_self = new ItemRepositoryRegistry;
  }
  return m_self;
}

void ItemRepositoryRegistry::deleteRepositoryFromDisk(ISession* session)
{
  QString repositoryPath = repositoryPathForSession(session);
  // Now, as we have only the global item-repository registry, assume that if and only if
  // the given session is ours, its cache path is used by the said global item-repository registry.
  if(m_self && m_self->d->m_path == repositoryPath) {
    m_self->d->m_shallDelete = true;
    return;
  }

  // Otherwise, given session is not ours.
  // Try to lock it; and, if locking succeeds, remove its item-repository directory directly.
  SessionController::LockSessionState state = SessionController::tryLockSession(session->id(), true);
  if(state) {
    removeDirectory(repositoryPath);

    // Unlock the session then.
    state.unlock();
  }
}

QMutex& ItemRepositoryRegistry::mutex()
{
  return d->m_mutex;
}

QAtomicInt& ItemRepositoryRegistry::getCustomCounter(const QString& identity, int initialValue)
{
  if(!d->m_customCounters.contains(identity))
    d->m_customCounters.insert(identity, new QAtomicInt(initialValue));
  return *d->m_customCounters[identity];
}

///The global item-repository registry that is used by default
ItemRepositoryRegistry& globalItemRepositoryRegistry()
{
  return *ItemRepositoryRegistry::self();
}

void ItemRepositoryRegistry::registerRepository(AbstractItemRepository* repository, AbstractRepositoryManager* manager)
{
  QMutexLocker lock(&d->m_mutex);
  d->m_repositories.insert(repository, manager);
  if(!d->m_path.isEmpty()) {
    if(!repository->open(d->m_path)) {
      d->deleteDataDirectory();
      kError() << "failed to open a repository";
      abort();
    }
  }
}

QString ItemRepositoryRegistry::path() const
{
  //We cannot lock the mutex here, since this may be called with one of the repositories locked,
  //and that may lead to a deadlock when at the same time a storing is requested
  return d->m_path;
}

void ItemRepositoryRegistryPrivate::lockForWriting()
{
  QMutexLocker lock(&m_mutex);
  //Create is_writing
  QFile f(m_path + "/is_writing");
  f.open(QIODevice::WriteOnly);
  f.close();
}

void ItemRepositoryRegistry::lockForWriting()
{
  d->lockForWriting();
}

void ItemRepositoryRegistryPrivate::unlockForWriting()
{
  QMutexLocker lock(&m_mutex);
  //Delete is_writing
  QFile::remove(m_path + "/is_writing");
}

void ItemRepositoryRegistry::unlockForWriting()
{
  d->unlockForWriting();
}

void ItemRepositoryRegistry::unRegisterRepository(AbstractItemRepository* repository)
{
  QMutexLocker lock(&d->m_mutex);
  Q_ASSERT(d->m_repositories.contains(repository));
  repository->close();
  d->m_repositories.remove(repository);
}

//After calling this, the data-directory may be a new one
void ItemRepositoryRegistryPrivate::deleteDataDirectory(bool recreate)
{
  QMutexLocker lock(&m_mutex);

  //lockForWriting creates a file, that prevents any other KDevelop instance from using the directory as it is.
  //Instead, the other instance will try to delete the directory as well.
  lockForWriting();

  bool result = removeDirectory(m_path);
  Q_ASSERT(result);
  Q_UNUSED(result);
  // Just recreate the directory then; leave old path (as it is dependent on appname and session only).
  if(recreate) {
    KStandardDirs::makeDir(m_path);
  }
}

bool ItemRepositoryRegistry::open(const QString& path, bool clear)
{
  QMutexLocker mlock(&d->m_mutex);
  if(d->m_path == path && !clear) {
    return true;
  }
  d->m_path = path;

  // Check if the repository shall be cleared
  // do-while is here for breaks
  do {
    if(QFile::exists(d->m_path + "/is_writing")) {
      kWarning() << "repository" << d->m_path << "was write-locked, it probably is inconsistent";
      clear = true;
      break;
    }

    if(!QFile::exists(d->m_path + QString("/version_%1").arg(staticItemRepositoryVersion()))) {
      kWarning() << "version-hint not found, seems to be an old version";
      clear = true;
      break;
    }

    if(getenv("CLEAR_DUCHAIN_DIR")) {
      kWarning() << "clearing duchain directory because CLEAR_DUCHAIN_DIR is set";
      clear = true;
      break;
    }

    QFile crashesFile(d->m_path + QString("/crash_counter"));
    if(crashesFile.open(QIODevice::ReadOnly)) {
      int count;
      QDataStream stream(&crashesFile);
      stream >> count;

      kDebug() << "current count of crashes: " << count;

      if(count >= crashesBeforeCleanup && !getenv("DONT_CLEAR_DUCHAIN_DIR")) {
        bool userAnswer = askUser(i18np("Session crashed %1 time in a row", "Session crashed %1 times in a row", count),
                                  i18nc("@action", "Clear cache"),
                                  i18nc("@title", "Session crashed"),
                                  i18n("The crash may be caused by a corruption of cached data.\n\n"
                                       "Press OK if you want KDevelop to clear the cache, otherwise press Cancel if you are sure the crash has another origin."));
        if(userAnswer) {
          clear = true;
          kDebug() << "User chose to clean repository";
          break;
        } else {
          setCrashCounter(crashesFile, 1);
          kDebug() << "User chose to reset crash counter";
        }
      } else {
        ///Increase the crash-count. It will be reset if kdevelop is shut down cleanly.
        setCrashCounter(crashesFile, ++count);
      }
    } else {
      setCrashCounter(crashesFile, 1);
    }
  } while(false);

  if(clear) {
    kWarning() << QString("The data-repository at %1 has to be cleared.").arg(d->m_path);
#ifdef Q_OS_WIN
    /// on Windows a file can't be deleted unless the last file handle gets closed
    /// deleteDataDirectory would enter a never ending loop
    crashesFile.close();
#endif
    d->deleteDataDirectory();
  }

  foreach(AbstractItemRepository* repository, d->m_repositories.keys()) {
    if(!repository->open(path)) {
      d->deleteDataDirectory();
      kError() << "failed to open a repository";
      abort();
    }
  }

  QFile f(path + "/Counters");
  if(f.open(QIODevice::ReadOnly)) {
    QDataStream stream(&f);

    while(!stream.atEnd()) {
      //Read in all custom counter values
      QString counterName;
      stream >> counterName;
      int counterValue;
      stream >> counterValue;
      getCustomCounter(counterName, 0) = counterValue;
    }
  }

  return true;
}

void ItemRepositoryRegistry::store()
{
  QMutexLocker lock(&d->m_mutex);
  foreach(AbstractItemRepository* repository, d->m_repositories.keys()) {
    repository->store();
  }

  QFile versionFile(d->m_path + QString("/version_%1").arg(staticItemRepositoryVersion()));
  if(versionFile.open(QIODevice::WriteOnly)) {
    versionFile.close();
  } else {
    kWarning() << "Could not open version file for writing";
  }

  //Store all custom counter values
  QFile f(d->m_path + "/Counters");
  if(f.open(QIODevice::WriteOnly)) {
    f.resize(0);
    QDataStream stream(&f);
    for(QMap<QString, QAtomicInt*>::const_iterator it = d->m_customCounters.constBegin();
        it != d->m_customCounters.constEnd();
        ++it) {
      stream << it.key();
      stream << it.value()->fetchAndAddRelaxed(0);
    }
  } else {
    kWarning() << "Could not open counter file for writing";
  }
}

void ItemRepositoryRegistry::printAllStatistics() const
{
  QMutexLocker lock(&d->m_mutex);
  foreach(AbstractItemRepository* repository, d->m_repositories.keys()) {
    kDebug() << "statistics in" << repository->repositoryName() << ":";
    kDebug() << repository->printStatistics();
  }
}

int ItemRepositoryRegistry::finalCleanup()
{
  QMutexLocker lock(&d->m_mutex);
  int changed = false;
  foreach(AbstractItemRepository* repository, d->m_repositories.keys()) {
    int added = repository->finalCleanup();
    changed += added;
    kDebug() << "cleaned in" << repository->repositoryName() << ":" << added;
  }
  return changed;
}

void ItemRepositoryRegistry::close()
{
  QMutexLocker lock(&d->m_mutex);

  foreach(AbstractItemRepository* repository, d->m_repositories.keys()) {
    repository->close();
  }

  d->m_path.clear();
}

ItemRepositoryRegistry::~ItemRepositoryRegistry()
{
  close();
  QMutexLocker lock(&d->m_mutex);
  foreach(QAtomicInt * counter, d->m_customCounters) {
    delete counter;
  }
  delete d;
}

void ItemRepositoryRegistry::shutdown()
{
  if(d->m_shallDelete) {
    d->deleteDataDirectory(false);
  } else {
    QFile::remove(d->m_path + QString("/crash_counter"));
  }
}

}

