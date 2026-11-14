cmake_minimum_required(VERSION 3.24)

project(PerfectAlarmClock VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(PerfectAlarmClock WIN32
    src/main.cpp
    src/core/Catalog.cpp
    src/core/Catalog.h
    src/core/DateMath.cpp
    src/core/DateMath.h
    src/core/Fonts.cpp
    src/core/Fonts.h
    src/core/Icons.cpp
    src/core/Icons.h
    src/core/Platform.cpp
    src/core/Platform.h
    src/core/Schedule.cpp
    src/core/Schedule.h
    src/core/Scheduler.cpp
    src/core/Scheduler.h
    src/core/Settings.cpp
    src/core/Settings.h
    src/core/TimerPlan.cpp
    src/core/TimerPlan.h
    src/core/SoundPlayer.cpp
    src/core/SoundPlayer.h
    src/core/Store.cpp
    src/core/Store.h
    src/core/Theme.cpp
    src/core/Theme.h
    src/core/Types.h
    src/ui/AlarmDelegate.cpp
    src/ui/AlarmDelegate.h
    src/ui/AlarmDialog.cpp
    src/ui/AlarmDialog.h
    src/ui/AlarmScheduleDialog.cpp
    src/ui/AlarmScheduleDialog.h
    src/ui/AlarmsPage.cpp
    src/ui/AlarmsPage.h
    src/ui/AlarmTableModel.cpp
    src/ui/AlarmTableModel.h
    src/ui/CalendarBoards.cpp
    src/ui/CalendarBoards.h
    src/ui/CalendarData.cpp
    src/ui/CalendarData.h
    src/ui/CalendarPage.cpp
    src/ui/CalendarPage.h
    src/ui/DateToolsDialog.cpp
    src/ui/DateToolsDialog.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/NavPane.cpp
    src/ui/NavPane.h
    src/ui/NonWorkingDaysDialog.cpp
    src/ui/NonWorkingDaysDialog.h
    src/ui/NotificationPopup.cpp
    src/ui/NotificationPopup.h
    src/ui/PomodoroPage.cpp
    src/ui/PomodoroPage.h
    src/ui/PomodoroSettingsDialog.cpp
    src/ui/PomodoroSettingsDialog.h
    src/ui/Ribbon.cpp
    src/ui/Ribbon.h
    src/ui/SettingsDialog.cpp
    src/ui/SettingsDialog.h
    src/ui/StopwatchPage.cpp
    src/ui/StopwatchPage.h
    src/ui/TimerDialog.cpp
    src/ui/TimerDialog.h
    src/ui/TimerStageDialog.cpp
    src/ui/TimerStageDialog.h
    src/ui/TimersPage.cpp
    src/ui/TimersPage.h
    src/ui/TodoDialog.cpp
    src/ui/TodoDialog.h
    src/ui/TodoPage.cpp
    src/ui/TodoPage.h
    src/ui/ThemeEffects.cpp
    src/ui/ThemeEffects.h
    src/ui/TitleBar.cpp
    src/ui/TitleBar.h
    src/ui/UiKit.cpp
    src/ui/UiKit.h
    src/ui/WorldClockPage.cpp
    src/ui/WorldClockPage.h
    resources.qrc
    app.rc
)

target_include_directories(PerfectAlarmClock PRIVATE src)

target_link_libraries(PerfectAlarmClock PRIVATE
    Qt6::Widgets
    winmm
)

if(MSVC)
    target_compile_options(PerfectAlarmClock PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus)
else()
    target_compile_options(PerfectAlarmClock PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(PerfectAlarmClock PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "PerfectAlarmClock"
)

install(TARGETS PerfectAlarmClock
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)
