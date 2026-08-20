cmake_minimum_required(VERSION 3.19)
project(OneClone LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Svg Network)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)



qt_add_executable(OneClone
    WIN32 MACOSX_BUNDLE
    src/main.cpp

    src/model/Config.h
    src/model/Job.h                     src/model/Job.cpp
    src/model/LocalPathAutocompleter.h  src/model/LocalPathAutocompleter.cpp
    src/model/LogFile.h                 src/model/LogFile.cpp
    src/model/Manager.h                 src/model/Manager.cpp
    src/model/RemotesAutocompleter.h    src/model/RemotesAutocompleter.cpp
    src/model/RemotesLookupWorker.h     src/model/RemotesLookupWorker.cpp
    src/model/UpdateManager.h           src/model/UpdateManager.cpp
    src/model/SharedSettings.h          src/model/SharedSettings.cpp

    src/view/MainWindow.h               src/view/MainWindow.cpp             src/view/MainWindow.ui
    src/view/JobWidget.h                src/view/JobWidget.cpp              src/view/JobWidget.ui
    src/view/JobListWidget.h            src/view/JobListWidget.cpp

    src/wizard/SetupWizard.h            src/wizard/SetupWizard.cpp
    src/wizard/SetupWizardPage1.h       src/wizard/SetupWizardPage1.cpp     src/wizard/SetupWizardPage1.ui
    src/wizard/SetupWizardPage2.h       src/wizard/SetupWizardPage2.cpp     src/wizard/SetupWizardPage2.ui
    src/wizard/SetupWizardPage3.h       src/wizard/SetupWizardPage3.cpp     src/wizard/SetupWizardPage3.ui

    resources/resources.qrc
    resources/main.rc
)

target_link_libraries(OneClone
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::Svg
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
