cmake_minimum_required(VERSION 3.19)
project(OneClone LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Svg SvgWidgets Network)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)



qt_add_executable(OneClone
    WIN32 MACOSX_BUNDLE

    src/app/main.cpp
    src/app/SingleInstanceGuard.h       src/app/SingleInstanceGuard.cpp

    src/common/Config.h
    src/common/LocalPathAutocompleter.h  src/common/LocalPathAutocompleter.cpp
    src/common/UpdateManager.h           src/common/UpdateManager.cpp

    src/core/AppContext.h              src/core/AppContext.cpp
    src/core/Job.h                     src/core/Job.cpp
    src/core/LogFile.h                 src/core/LogFile.cpp
    src/core/SharedSettings.h          src/core/SharedSettings.cpp
    src/core/Status.h

    src/providers/platform/AutostartManager.h       src/providers/platform/AutostartManager.cpp
    src/providers/platform/MountBackendDetector.h
    src/providers/platform/WinFspDetector.h         src/providers/platform/WinFspDetector.cpp

    src/providers/rclone/RCloneProvider.h           src/providers/rclone/RCloneProvider.cpp
    src/providers/rclone/RCloneProviderWindows.h    src/providers/rclone/RCloneProviderWindows.cpp
    src/providers/rclone/RemotesAutocompleter.h     src/providers/rclone/RemotesAutocompleter.cpp
    src/providers/rclone/RemotesLookupWorker.h      src/providers/rclone/RemotesLookupWorker.cpp

    src/ui/controllers/JobDetailsTabController.h    src/ui/controllers/JobDetailsTabController.cpp
    src/ui/controllers/JobsTabController.h          src/ui/controllers/JobsTabController.cpp
    src/ui/controllers/SettingsTabController.h      src/ui/controllers/SettingsTabController.cpp
    src/ui/controllers/TrayController.h             src/ui/controllers/TrayController.cpp

    src/ui/widgets/JobListWidget.h          src/ui/widgets/JobListWidget.cpp
    src/ui/widgets/JobWidget.h              src/ui/widgets/JobWidget.cpp        src/ui/widgets/JobWidget.ui
    src/ui/widgets/QLabelInstantTooltip.h

    src/ui/wizard/SetupWizard.h         src/ui/wizard/SetupWizard.cpp
    src/ui/wizard/SetupWizardPage0.h    src/ui/wizard/SetupWizardPage0.cpp  src/ui/wizard/SetupWizardPage0.ui
    src/ui/wizard/SetupWizardPage1.h    src/ui/wizard/SetupWizardPage1.cpp  src/ui/wizard/SetupWizardPage1.ui
    src/ui/wizard/SetupWizardPage2.h    src/ui/wizard/SetupWizardPage2.cpp  src/ui/wizard/SetupWizardPage2.ui
    src/ui/wizard/SetupWizardPage3.h    src/ui/wizard/SetupWizardPage3.cpp  src/ui/wizard/SetupWizardPage3.ui

    src/ui/MainWindow.h                     src/ui/MainWindow.cpp               src/ui/MainWindow.ui

    resources/resources.qrc
    resources/main.rc
)

target_link_libraries(OneClone
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::Svg
        Qt::SvgWidgets
)

install(TARGETS OneClone
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET OneClone
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

target_include_directories(OneClone PRIVATE ${CMAKE_SOURCE_DIR})

install(SCRIPT ${deploy_script})
