/* KDevelop Standard OutputView
 *
 * Copyright 2014 Alex Richardson <arichardson.kde@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "standardoutputview.h"

#include <KPluginFactory>

// this is split out to a separate file so that compiling the test doesn't need the json file
K_PLUGIN_FACTORY_WITH_JSON(StandardOutputViewFactory, "kdevstandardoutputview.json", registerPlugin<StandardOutputView>(); )

#include "standardoutputviewmetadata.moc"
