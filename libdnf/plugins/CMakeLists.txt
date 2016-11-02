if (ENABLE_SUBSCRIPTION_MANAGER_PLUGIN)
  pkg_check_modules (RHSM REQUIRED librhsm)

  add_library (subscription-manager-plugin SHARED subscription-manager-plugin.c)
  target_link_libraries (subscription-manager-plugin
                         ${RHSM_LIBRARIES}
                         ${PEAS_LIBRARIES}
                         libdnf)
  target_include_directories (subscription-manager-plugin
                              PRIVATE ${RHSM_INCLUDE_DIRS}
                              PRIVATE ${PEAS_INCLUDE_DIRS}
                              PRIVATE ..)
  set_target_properties (subscription-manager-plugin
                         PROPERTIES OUTPUT_NAME "subscription-manager")

  install (TARGETS subscription-manager-plugin
           LIBRARY DESTINATION ${PKG_LIBDIR}/plugins)
  install (FILES subscription-manager.plugin
           DESTINATION ${PKG_LIBDIR}/plugins)
endif ()
