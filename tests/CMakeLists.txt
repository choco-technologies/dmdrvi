set(DMOD_MODULE_NAME        test_dmdrvi)
set(DMOD_MODULE_VERSION     ${PROJECT_VERSION})
set(DMOD_AUTHOR_NAME        "Patryk Kubiak")
set(DMOD_STACK_SIZE         1024)
set(DMOD_PRIORITY           0)

dmod_add_test(${DMOD_MODULE_NAME} ${DMOD_MODULE_VERSION}
    dmdrvi_contract_test.c
)

target_link_libraries(${DMOD_MODULE_NAME} dmdrvi_if)

dmod_link_modules(${DMOD_MODULE_NAME}
    dmini
)
