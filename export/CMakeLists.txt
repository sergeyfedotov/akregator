include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )

set(akregatorstorageexporter_SRCS akregatorstorageexporter.cpp)

add_executable(akregatorstorageexporter ${akregatorstorageexporter_SRCS})

target_link_libraries(akregatorstorageexporter
    KF5::Syndication
    akregatorinterfaces
    KF5::Service
    )

install(TARGETS akregatorstorageexporter ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

