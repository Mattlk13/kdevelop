set(krunner_kdevelopsessions_SRCS
    kdevelopsessions.cpp
)

add_library(krunner_kdevelopsessions MODULE ${krunner_kdevelopsessions_SRCS})
target_link_libraries(krunner_kdevelopsessions KF5::I18n KF5::Runner KF5::KIOWidgets)

install(TARGETS krunner_kdevelopsessions DESTINATION ${KDE_INSTALL_PLUGINDIR} )

install(FILES kdevelopsessions.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

