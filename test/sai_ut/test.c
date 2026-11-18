#include <stdio.h>
#include <string.h>
#include <stdbool.h>
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

static bool port_list_contains(
    const sai_object_id_t *ports,
    uint32_t count,
    sai_object_id_t expected)
{
    uint32_t ii;

    for (ii = 0; ii < count; ii++) {
        if (ports[ii] == expected) {
            return true;
        }
    }

    return false;
}

static sai_status_t get_lag_port_list(
    sai_lag_api_t *lag_api,
    sai_object_id_t lag_oid,
    sai_object_id_t *ports,
    uint32_t max_ports,
    uint32_t *out_count)
{
    sai_attribute_t attr;
    sai_status_t status;

    memset(ports, 0, sizeof(sai_object_id_t) * max_ports);
    attr.id = SAI_LAG_ATTR_PORT_LIST;
    attr.value.objlist.list = ports;
    attr.value.objlist.count = max_ports;

    status = lag_api->get_lag_attribute(lag_oid, 1, &attr);
    if (status == SAI_STATUS_SUCCESS) {
        *out_count = attr.value.objlist.count;
    }

    return status;
}

static sai_status_t get_lag_member_attr(
    sai_lag_api_t *lag_api,
    sai_object_id_t member_oid,
    sai_attr_id_t attr_id,
    sai_object_id_t *out_oid)
{
    sai_attribute_t attr;
    sai_status_t status;

    memset(&attr, 0, sizeof(attr));
    attr.id = attr_id;

    status = lag_api->get_lag_member_attribute(member_oid, 1, &attr);
    if (status == SAI_STATUS_SUCCESS) {
        *out_oid = attr.value.oid;
    }

    return status;
}

/* Happy-path flow from homework: create, get, remove member, verify PORT_LIST. */
int test_LAG_api(sai_object_id_t port1,
                 sai_object_id_t port2,
                 sai_object_id_t port3,
                 sai_object_id_t port4)
{
    sai_status_t status;
    sai_lag_api_t *lag_api;
    sai_object_id_t lag1, lag2;
    sai_object_id_t member1, member2, member3, member4;
    sai_object_id_t ports[8];
    sai_object_id_t got_oid;
    uint32_t count = 0;

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
    status = get_lag_port_list(lag_api, lag1, ports, 8, &count);
    if (status != SAI_STATUS_SUCCESS || count != 2 ||
        !port_list_contains(ports, count, port1) ||
        !port_list_contains(ports, count, port2)) {
        printf("Unexpected LAG#1 PORT_LIST, status=%d count=%u\n", status, count);
        return 1;
    }

    /* Get LAG#2 PORT_LIST [Expected: (PORT#3, PORT#4)] */
    status = get_lag_port_list(lag_api, lag2, ports, 8, &count);
    if (status != SAI_STATUS_SUCCESS || count != 2 ||
        !port_list_contains(ports, count, port3) ||
        !port_list_contains(ports, count, port4)) {
        printf("Unexpected LAG#2 PORT_LIST, status=%d count=%u\n", status, count);
        return 1;
    }

    /* Get LAG_MEMBER#1 LAG_ID [Expected: LAG#1] */
    status = get_lag_member_attr(lag_api, member1, SAI_LAG_MEMBER_ATTR_LAG_ID, &got_oid);
    if (status != SAI_STATUS_SUCCESS || got_oid != lag1) {
        printf("Unexpected LAG_MEMBER#1 LAG_ID, status=%d\n", status);
        return 1;
    }

    /* Get LAG_MEMBER#3 PORT_ID [Expected: PORT#3] */
    status = get_lag_member_attr(lag_api, member3, SAI_LAG_MEMBER_ATTR_PORT_ID, &got_oid);
    if (status != SAI_STATUS_SUCCESS || got_oid != port3) {
        printf("Unexpected LAG_MEMBER#3 PORT_ID, status=%d\n", status);
        return 1;
    }

    /* Remove LAG with members must fail */
    status = lag_api->remove_lag(lag1);
    if (status == SAI_STATUS_SUCCESS) {
        printf("Removing LAG#1 with members unexpectedly succeeded\n");
        return 1;
    }

    /* Remove LAG_MEMBER#2 */
    status = lag_api->remove_lag_member(member2);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove LAG_MEMBER#2, status=%d\n", status);
        return 1;
    }

    /* Get LAG#1 PORT_LIST [Expected: (PORT#1)] */
    status = get_lag_port_list(lag_api, lag1, ports, 8, &count);
    if (status != SAI_STATUS_SUCCESS || count != 1 || ports[0] != port1) {
        printf("Unexpected LAG#1 PORT_LIST after remove, status=%d count=%u\n", status, count);
        return 1;
    }

    /* Remove LAG_MEMBER#3 */
    status = lag_api->remove_lag_member(member3);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove LAG_MEMBER#3, status=%d\n", status);
        return 1;
    }

    /* Get LAG#2 PORT_LIST [Expected: (PORT#4)] */
    status = get_lag_port_list(lag_api, lag2, ports, 8, &count);
    if (status != SAI_STATUS_SUCCESS || count != 1 || ports[0] != port4) {
        printf("Unexpected LAG#2 PORT_LIST after remove, status=%d count=%u\n", status, count);
        return 1;
    }

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

    /* Empty PORT_LIST after all members removed */
    status = get_lag_port_list(lag_api, lag1, ports, 8, &count);
    if (status != SAI_STATUS_SUCCESS || count != 0) {
        printf("Unexpected empty LAG#1 PORT_LIST, status=%d count=%u\n", status, count);
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

/* Limits: up to 5 LAGs and up to 16 LAG_MEMBERs; mandatory attrs; slot reuse. */
int test_LAG_limits_and_attrs(sai_object_id_t *port_list, uint32_t port_count)
{
    sai_status_t status;
    sai_lag_api_t *lag_api;
    sai_object_id_t lags[6];
    sai_object_id_t members[17];
    sai_object_id_t extra_lag;
    sai_object_id_t got_oid;
    sai_attribute_t attrs[2];
    uint32_t ii;

    if (port_count < 1) {
        printf("Need at least 1 port for limits test\n");
        return 1;
    }

    status = sai_api_query(SAI_API_LAG, (void**)&lag_api);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to query LAG API, status=%d\n", status);
        return 1;
    }

    /* Create max LAGs (5) */
    for (ii = 0; ii < 5; ii++) {
        status = lag_api->create_lag(&lags[ii], 0, NULL);
        if (status != SAI_STATUS_SUCCESS) {
            printf("Failed to create LAG[%u], status=%d\n", ii, status);
            return 1;
        }
    }

    /* 6th LAG must fail */
    status = lag_api->create_lag(&lags[5], 0, NULL);
    if (status == SAI_STATUS_SUCCESS) {
        printf("Creating 6th LAG unexpectedly succeeded\n");
        return 1;
    }

    /* Free one LAG and reuse the slot */
    status = lag_api->remove_lag(lags[0]);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove LAG[0] for reuse, status=%d\n", status);
        return 1;
    }

    status = lag_api->create_lag(&extra_lag, 0, NULL);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to recreate LAG after free, status=%d\n", status);
        return 1;
    }
    lags[0] = extra_lag;

    /* Missing mandatory LAG_MEMBER attributes must fail */
    status = lag_api->create_lag_member(&members[0], 0, NULL);
    if (status == SAI_STATUS_SUCCESS) {
        printf("Creating LAG_MEMBER without attrs unexpectedly succeeded\n");
        return 1;
    }

    attrs[0].id = SAI_LAG_MEMBER_ATTR_LAG_ID;
    attrs[0].value.oid = lags[0];
    status = lag_api->create_lag_member(&members[0], 1, attrs);
    if (status == SAI_STATUS_SUCCESS) {
        printf("Creating LAG_MEMBER without PORT_ID unexpectedly succeeded\n");
        return 1;
    }

    /* Create max LAG_MEMBERs (16) on LAG[0] */
    for (ii = 0; ii < 16; ii++) {
        status = create_lag_member(lag_api, &members[ii], lags[0], port_list[ii % port_count]);
        if (status != SAI_STATUS_SUCCESS) {
            printf("Failed to create LAG_MEMBER[%u], status=%d\n", ii, status);
            return 1;
        }
    }

    /* 17th LAG_MEMBER must fail */
    status = create_lag_member(lag_api, &members[16], lags[0], port_list[0]);
    if (status == SAI_STATUS_SUCCESS) {
        printf("Creating 17th LAG_MEMBER unexpectedly succeeded\n");
        return 1;
    }

    /* Verify get on first/last member */
    status = get_lag_member_attr(lag_api, members[0], SAI_LAG_MEMBER_ATTR_LAG_ID, &got_oid);
    if (status != SAI_STATUS_SUCCESS || got_oid != lags[0]) {
        printf("Unexpected members[0] LAG_ID\n");
        return 1;
    }

    status = get_lag_member_attr(lag_api, members[15], SAI_LAG_MEMBER_ATTR_PORT_ID, &got_oid);
    if (status != SAI_STATUS_SUCCESS || got_oid != port_list[15 % port_count]) {
        printf("Unexpected members[15] PORT_ID\n");
        return 1;
    }

    /* Cleanup members then LAGs */
    for (ii = 0; ii < 16; ii++) {
        status = lag_api->remove_lag_member(members[ii]);
        if (status != SAI_STATUS_SUCCESS) {
            printf("Failed to remove LAG_MEMBER[%u], status=%d\n", ii, status);
            return 1;
        }
    }

    for (ii = 0; ii < 5; ii++) {
        status = lag_api->remove_lag(lags[ii]);
        if (status != SAI_STATUS_SUCCESS) {
            printf("Failed to remove LAG[%u], status=%d\n", ii, status);
            return 1;
        }
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
    int failed = 0;

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
    if (status != 0) {
        failed = 1;
    }

    status = test_LAG_limits_and_attrs(port_list, attrs[0].value.objlist.count);
    printf("test_LAG_limits_and_attrs status %d\n", status);
    if (status != 0) {
        failed = 1;
    }

    switch_api->shutdown_switch(0);
    sai_api_uninitialize();

    return failed;
}
