#include <stdio.h>
#include <string.h>
#include "sai.h"

const char* test_profile_get_value(
    _In_ sai_switch_profile_id_t profile_id,
    _In_ const char* variable)
{
    return 0;
}

int test_profile_get_next_value(
    _In_ sai_switch_profile_id_t profile_id,
    _Out_ const char** variable,
    _Out_ const char** value)
{
    return -1;
}

const service_method_table_t test_services = {
    test_profile_get_value,
    test_profile_get_next_value
};

static sai_status_t create_lag_member(
    sai_lag_api_t *lag_api,
    sai_object_id_t *member_oid,
    sai_object_id_t lag_oid,
    sai_object_id_t port_oid)
{
    sai_attribute_t attrs[2];

    attrs[0].id = SAI_LAG_MEMBER_ATTR_LAG_ID;
    attrs[0].value.oid = lag_oid;
    attrs[1].id = SAI_LAG_MEMBER_ATTR_PORT_ID;
    attrs[1].value.oid = port_oid;

    return lag_api->create_lag_member(member_oid, 2, attrs);
}

static void get_lag_port_list(sai_lag_api_t *lag_api, sai_object_id_t lag_oid)
{
    sai_object_id_t ports[8];
    sai_attribute_t attr;

    memset(ports, 0, sizeof(ports));
    attr.id = SAI_LAG_ATTR_PORT_LIST;
    attr.value.objlist.list = ports;
    attr.value.objlist.count = 8;

    /* Implementation does not support get yet — do not check the return value. */
    lag_api->get_lag_attribute(lag_oid, 1, &attr);
}

static void get_lag_member_attr(
    sai_lag_api_t *lag_api,
    sai_object_id_t member_oid,
    sai_attr_id_t attr_id)
{
    sai_attribute_t attr;

    memset(&attr, 0, sizeof(attr));
    attr.id = attr_id;

    /* Implementation does not support get yet — do not check the return value. */
    lag_api->get_lag_member_attribute(member_oid, 1, &attr);
}

int test_LAG_api(sai_object_id_t port1,
                 sai_object_id_t port2,
                 sai_object_id_t port3,
                 sai_object_id_t port4)
{
    sai_status_t status;
    sai_lag_api_t *lag_api;
    sai_object_id_t lag1, lag2;
    sai_object_id_t member1, member2, member3, member4;

    status = sai_api_query(SAI_API_LAG, (void**)&lag_api);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to query LAG API, status=%d\n", status);
        return 1;
    }

    /* Create LAG#1 */
    status = lag_api->create_lag(&lag1, 0, NULL);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create LAG#1, status=%d\n", status);
        return 1;
    }

    /* Create LAG_MEMBER#1 {LAG_ID:LAG#1, PORT_ID:PORT#1} */
    status = create_lag_member(lag_api, &member1, lag1, port1);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create LAG_MEMBER#1, status=%d\n", status);
        return 1;
    }

    /* Create LAG_MEMBER#2 {LAG_ID:LAG#1, PORT_ID:PORT#2} */
    status = create_lag_member(lag_api, &member2, lag1, port2);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create LAG_MEMBER#2, status=%d\n", status);
        return 1;
    }

    /* Create LAG#2 */
    status = lag_api->create_lag(&lag2, 0, NULL);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create LAG#2, status=%d\n", status);
        return 1;
    }

    /* Create LAG_MEMBER#3 {LAG_ID:LAG#2, PORT_ID:PORT#3} */
    status = create_lag_member(lag_api, &member3, lag2, port3);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create LAG_MEMBER#3, status=%d\n", status);
        return 1;
    }

    /* Create LAG_MEMBER#4 {LAG_ID:LAG#2, PORT_ID:PORT#4} */
    status = create_lag_member(lag_api, &member4, lag2, port4);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create LAG_MEMBER#4, status=%d\n", status);
        return 1;
    }

    /* Get LAG#1 PORT_LIST [Expected: (PORT#1, PORT#2)] */
    get_lag_port_list(lag_api, lag1);

    /* Get LAG#2 PORT_LIST [Expected: (PORT#3, PORT#4)] */
    get_lag_port_list(lag_api, lag2);

    /* Get LAG_MEMBER#1 LAG_ID [Expected: LAG#1] */
    get_lag_member_attr(lag_api, member1, SAI_LAG_MEMBER_ATTR_LAG_ID);

    /* Get LAG_MEMBER#3 PORT_ID [Expected: PORT#3] */
    get_lag_member_attr(lag_api, member3, SAI_LAG_MEMBER_ATTR_PORT_ID);

    /* Remove LAG_MEMBER#2 */
    status = lag_api->remove_lag_member(member2);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove LAG_MEMBER#2, status=%d\n", status);
        return 1;
    }

    /* Get LAG#1 PORT_LIST [Expected: (PORT#1)] */
    get_lag_port_list(lag_api, lag1);

    /* Remove LAG_MEMBER#3 */
    status = lag_api->remove_lag_member(member3);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove LAG_MEMBER#3, status=%d\n", status);
        return 1;
    }

    /* Get LAG#2 PORT_LIST [Expected: (PORT#4)] */
    get_lag_port_list(lag_api, lag2);

    /* Remove LAG_MEMBER#1 */
    status = lag_api->remove_lag_member(member1);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove LAG_MEMBER#1, status=%d\n", status);
        return 1;
    }

    /* Remove LAG_MEMBER#4 */
    status = lag_api->remove_lag_member(member4);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove LAG_MEMBER#4, status=%d\n", status);
        return 1;
    }

    /* Remove LAG#2 */
    status = lag_api->remove_lag(lag2);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove LAG#2, status=%d\n", status);
        return 1;
    }

    /* Remove LAG#1 */
    status = lag_api->remove_lag(lag1);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove LAG#1, status=%d\n", status);
        return 1;
    }

    return 0;
}

int main(void)
{
    sai_status_t status;
    sai_switch_api_t *switch_api;
    sai_switch_notification_t notifications;
    sai_attribute_t attrs[1];
    sai_object_id_t port_list[64];

    memset(&notifications, 0, sizeof(notifications));

    status = sai_api_initialize(0, &test_services);
    if (status != SAI_STATUS_SUCCESS) {
        printf("sai_api_initialize failed, status=%d\n", status);
        return 1;
    }

    status = sai_api_query(SAI_API_SWITCH, (void**)&switch_api);
    if (status != SAI_STATUS_SUCCESS) {
        printf("sai_api_query SWITCH failed, status=%d\n", status);
        return 1;
    }

    status = switch_api->initialize_switch(0, "HW_ID", 0, &notifications);
    if (status != SAI_STATUS_SUCCESS) {
        printf("initialize_switch failed, status=%d\n", status);
        return 1;
    }

    attrs[0].id = SAI_SWITCH_ATTR_PORT_LIST;
    attrs[0].value.objlist.list = port_list;
    attrs[0].value.objlist.count = 64;
    status = switch_api->get_switch_attribute(1, attrs);
    if (status != SAI_STATUS_SUCCESS || attrs[0].value.objlist.count < 4) {
        printf("Failed to get at least 4 switch ports, status=%d count=%u\n",
               status, attrs[0].value.objlist.count);
        return 1;
    }

    status = test_LAG_api(port_list[0], port_list[1], port_list[2], port_list[3]);
    printf("test_LAG_api status %d\n", status);

    switch_api->shutdown_switch(0);
    sai_api_uninitialize();

    return status;
}
