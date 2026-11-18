#include "sai.h"
#include "stub_sai.h"
#include "assert.h"

#define MAX_NUMBER_OF_LAG_MEMBERS 16
#define MAX_NUMBER_OF_LAGS 5

typedef struct _lag_member_db_entry_t {
    bool            is_used;
    sai_object_id_t port_oid;
    sai_object_id_t lag_oid;
} lag_member_db_entry_t;

typedef struct _lag_db_entry_t {
    bool            is_used;
    sai_object_id_t members_ids[MAX_NUMBER_OF_LAG_MEMBERS];
} lag_db_entry_t;

struct lag_db_t {
    lag_db_entry_t        lags[MAX_NUMBER_OF_LAGS];
    lag_member_db_entry_t members[MAX_NUMBER_OF_LAG_MEMBERS];
} lag_db;

static const sai_attribute_entry_t lag_attribs[] = {
    { SAI_LAG_ATTR_PORT_LIST, false, false, false, true,
      "List of ports in LAG", SAI_ATTR_VAL_TYPE_OBJLIST },
    { END_FUNCTIONALITY_ATTRIBS_ID, false, false, false, false,
      "", SAI_ATTR_VAL_TYPE_UNDETERMINED }
};

static const sai_attribute_entry_t lag_member_attribs[] = {
    { SAI_LAG_MEMBER_ATTR_LAG_ID, true, true, false, true,
      "LAG ID", SAI_ATTR_VAL_TYPE_OID },
    { SAI_LAG_MEMBER_ATTR_PORT_ID, true, true, false, true,
      "PORT ID", SAI_ATTR_VAL_TYPE_OID },
    { END_FUNCTIONALITY_ATTRIBS_ID, false, false, false, false,
      "", SAI_ATTR_VAL_TYPE_UNDETERMINED }
};

sai_status_t get_lag_attribute(_In_ const sai_object_key_t   *key,
                               _Inout_ sai_attribute_value_t *value,
                               _In_ uint32_t                  attr_index,
                               _Inout_ vendor_cache_t        *cache,
                               void                          *arg);

sai_status_t get_lag_member_attribute(_In_ const sai_object_key_t   *key,
                                      _Inout_ sai_attribute_value_t *value,
                                      _In_ uint32_t                  attr_index,
                                      _Inout_ vendor_cache_t        *cache,
                                      void                          *arg);

static const sai_vendor_attribute_entry_t lag_vendor_attribs[] = {
    { SAI_LAG_ATTR_PORT_LIST,
      { false, false, false, true },
      { false, false, false, true },
      get_lag_attribute, (void*)SAI_LAG_ATTR_PORT_LIST,
      NULL, NULL }
};

static const sai_vendor_attribute_entry_t lag_member_vendor_attribs[] = {
    { SAI_LAG_MEMBER_ATTR_LAG_ID,
      { true, false, false, true },
      { true, false, false, true },
      get_lag_member_attribute, (void*)SAI_LAG_MEMBER_ATTR_LAG_ID,
      NULL, NULL },
    { SAI_LAG_MEMBER_ATTR_PORT_ID,
      { true, false, false, true },
      { true, false, false, true },
      get_lag_member_attribute, (void*)SAI_LAG_MEMBER_ATTR_PORT_ID,
      NULL, NULL }
};

sai_status_t get_lag_member_attribute(_In_ const sai_object_key_t   *key,
                                      _Inout_ sai_attribute_value_t *value,
                                      _In_ uint32_t                  attr_index,
                                      _Inout_ vendor_cache_t        *cache,
                                      void                          *arg)
{
    sai_status_t status;
    uint32_t     db_index;

    assert((SAI_LAG_MEMBER_ATTR_LAG_ID == (int64_t)arg) ||
           (SAI_LAG_MEMBER_ATTR_PORT_ID == (int64_t)arg));

    status = stub_object_to_type(key->object_id, SAI_OBJECT_TYPE_LAG_MEMBER, &db_index);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG MEMBER DB index.\n");
        return status;
    }

    if (!lag_db.members[db_index].is_used) {
        printf("LAG MEMBER DB entry is not used.\n");
        return SAI_STATUS_INVALID_OBJECT_ID;
    }

    switch ((int64_t)arg) {
    case SAI_LAG_MEMBER_ATTR_LAG_ID:
        value->oid = lag_db.members[db_index].lag_oid;
        break;
    case SAI_LAG_MEMBER_ATTR_PORT_ID:
        value->oid = lag_db.members[db_index].port_oid;
        break;
    default:
        printf("Got unexpected attribute ID\n");
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t get_lag_attribute(_In_ const sai_object_key_t   *key,
                               _Inout_ sai_attribute_value_t *value,
                               _In_ uint32_t                  attr_index,
                               _Inout_ vendor_cache_t        *cache,
                               void                          *arg)
{
    sai_status_t    status;
    uint32_t        lag_db_id;
    uint32_t        ii;
    uint32_t        member_db_id;
    uint32_t        port_count = 0;
    sai_object_id_t ports[MAX_NUMBER_OF_LAG_MEMBERS];

    assert(SAI_LAG_ATTR_PORT_LIST == (int64_t)arg);

    status = stub_object_to_type(key->object_id, SAI_OBJECT_TYPE_LAG, &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG DB index.\n");
        return status;
    }

    if (!lag_db.lags[lag_db_id].is_used) {
        printf("LAG DB entry is not used.\n");
        return SAI_STATUS_INVALID_OBJECT_ID;
    }

    for (ii = 0; ii < MAX_NUMBER_OF_LAG_MEMBERS; ii++) {
        if (0 == lag_db.lags[lag_db_id].members_ids[ii]) {
            continue;
        }

        status = stub_object_to_type(lag_db.lags[lag_db_id].members_ids[ii],
                                     SAI_OBJECT_TYPE_LAG_MEMBER,
                                     &member_db_id);
        if (status != SAI_STATUS_SUCCESS) {
            printf("Cannot get LAG MEMBER DB index from LAG members list.\n");
            return status;
        }

        if (!lag_db.members[member_db_id].is_used) {
            continue;
        }

        ports[port_count++] = lag_db.members[member_db_id].port_oid;
    }

    return stub_fill_objlist(ports, port_count, &value->objlist);
}

sai_status_t stub_create_lag(
    _Out_ sai_object_id_t* lag_id,
    _In_ uint32_t attr_count,
    _In_ sai_attribute_t *attr_list)
{
    sai_status_t status;
    uint32_t     ii = 0;
    uint32_t     lag_db_id;
    char         list_str[MAX_LIST_VALUE_STR_LEN];

    if (NULL == lag_id) {
        return SAI_STATUS_INVALID_PARAMETER;
    }

    status = check_attribs_metadata(attr_count, attr_list, lag_attribs,
                                    lag_vendor_attribs, SAI_OPERATION_CREATE);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed attributes check\n");
        return status;
    }

    sai_attr_list_to_str(attr_count, attr_list, lag_attribs, MAX_LIST_VALUE_STR_LEN, list_str);

    for (; ii < MAX_NUMBER_OF_LAGS; ii++) {
        if (!lag_db.lags[ii].is_used) {
            break;
        }
    }
    if (ii == MAX_NUMBER_OF_LAGS) {
        printf("Cannot create LAG: limit is reached\n");
        return SAI_STATUS_FAILURE;
    }

    lag_db_id = ii;
    memset(&lag_db.lags[lag_db_id], 0, sizeof(lag_db.lags[lag_db_id]));
    lag_db.lags[lag_db_id].is_used = true;

    status = stub_create_object(SAI_OBJECT_TYPE_LAG, lag_db_id, lag_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot create a LAG OID\n");
        lag_db.lags[lag_db_id].is_used = false;
        return status;
    }

    printf("CREATE LAG: 0x%lX (%s)\n", *lag_id, list_str);
    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_remove_lag(
    _In_ sai_object_id_t  lag_id)
{
    sai_status_t status;
    uint32_t     lag_db_id;
    uint32_t     ii;

    status = stub_object_to_type(lag_id, SAI_OBJECT_TYPE_LAG, &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG DB ID.\n");
        return status;
    }

    if (!lag_db.lags[lag_db_id].is_used) {
        printf("Cannot remove LAG: DB entry is not used.\n");
        return SAI_STATUS_INVALID_OBJECT_ID;
    }

    for (ii = 0; ii < MAX_NUMBER_OF_LAG_MEMBERS; ii++) {
        if (0 != lag_db.lags[lag_db_id].members_ids[ii]) {
            printf("Cannot remove LAG: it still has LAG_MEMBERs\n");
            return SAI_STATUS_FAILURE;
        }
    }

    lag_db.lags[lag_db_id].is_used = false;
    memset(lag_db.lags[lag_db_id].members_ids, 0, sizeof(lag_db.lags[lag_db_id].members_ids));

    printf("REMOVE LAG: 0x%lX\n", lag_id);
    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_set_lag_attribute(
    _In_ sai_object_id_t  lag_id,
    _In_ const sai_attribute_t *attr)
{
    printf("SET LAG ATTRIBUTE is being called\n");
    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_get_lag_attribute(
    _In_ sai_object_id_t lag_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list)
{
    const sai_object_key_t key = { .object_id = lag_id };
    return sai_get_attributes(&key, NULL, lag_attribs, lag_vendor_attribs, attr_count, attr_list);
}

sai_status_t stub_create_lag_member(
    _Out_ sai_object_id_t* lag_member_id,
    _In_ uint32_t attr_count,
    _In_ sai_attribute_t *attr_list)
{
    sai_status_t                 status;
    uint32_t                     ii = 0;
    uint32_t                     lag_member_db_id;
    uint32_t                     lag_db_id;
    uint32_t                     member_slot;
    const sai_attribute_value_t *lag_id;
    const sai_attribute_value_t *port_id;
    uint32_t                     lag_id_idx;
    uint32_t                     port_id_idx;
    char                         list_str[MAX_LIST_VALUE_STR_LEN];

    if (NULL == lag_member_id) {
        return SAI_STATUS_INVALID_PARAMETER;
    }

    status = check_attribs_metadata(attr_count, attr_list, lag_member_attribs,
                                    lag_member_vendor_attribs, SAI_OPERATION_CREATE);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed attributes check\n");
        return status;
    }

    sai_attr_list_to_str(attr_count, attr_list, lag_member_attribs, MAX_LIST_VALUE_STR_LEN, list_str);

    status = find_attrib_in_list(attr_count, attr_list, SAI_LAG_MEMBER_ATTR_LAG_ID, &lag_id, &lag_id_idx);
    if (status != SAI_STATUS_SUCCESS) {
        printf("LAG_ID attribute not found.\n");
        return status;
    }

    status = find_attrib_in_list(attr_count, attr_list, SAI_LAG_MEMBER_ATTR_PORT_ID, &port_id, &port_id_idx);
    if (status != SAI_STATUS_SUCCESS) {
        printf("PORT_ID attribute not found.\n");
        return status;
    }

    status = stub_object_to_type(lag_id->oid, SAI_OBJECT_TYPE_LAG, &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG DB ID from LAG_ID attribute.\n");
        return status;
    }

    if (!lag_db.lags[lag_db_id].is_used) {
        printf("Cannot create LAG MEMBER: LAG is not used.\n");
        return SAI_STATUS_INVALID_OBJECT_ID;
    }

    for (; ii < MAX_NUMBER_OF_LAG_MEMBERS; ii++) {
        if (!lag_db.members[ii].is_used) {
            break;
        }
    }
    if (ii == MAX_NUMBER_OF_LAG_MEMBERS) {
        printf("Cannot create LAG MEMBER: limit is reached\n");
        return SAI_STATUS_FAILURE;
    }

    for (member_slot = 0; member_slot < MAX_NUMBER_OF_LAG_MEMBERS; member_slot++) {
        if (0 == lag_db.lags[lag_db_id].members_ids[member_slot]) {
            break;
        }
    }
    if (member_slot == MAX_NUMBER_OF_LAG_MEMBERS) {
        printf("Cannot create LAG MEMBER: LAG members list is full\n");
        return SAI_STATUS_FAILURE;
    }

    lag_member_db_id = ii;
    memset(&lag_db.members[lag_member_db_id], 0, sizeof(lag_db.members[lag_member_db_id]));
    lag_db.members[lag_member_db_id].is_used = true;
    lag_db.members[lag_member_db_id].lag_oid = lag_id->oid;
    lag_db.members[lag_member_db_id].port_oid = port_id->oid;

    status = stub_create_object(SAI_OBJECT_TYPE_LAG_MEMBER, lag_member_db_id, lag_member_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot create a LAG MEMBER OID\n");
        lag_db.members[lag_member_db_id].is_used = false;
        return status;
    }

    lag_db.lags[lag_db_id].members_ids[member_slot] = *lag_member_id;

    printf("CREATE LAG MEMBER: 0x%lX (%s)\n", *lag_member_id, list_str);
    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_remove_lag_member(
    _In_ sai_object_id_t  lag_member_id)
{
    sai_status_t status;
    uint32_t     lag_member_db_id;
    uint32_t     lag_db_id;
    uint32_t     ii;

    status = stub_object_to_type(lag_member_id, SAI_OBJECT_TYPE_LAG_MEMBER, &lag_member_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG MEMBER DB ID.\n");
        return status;
    }

    if (!lag_db.members[lag_member_db_id].is_used) {
        printf("Cannot remove LAG MEMBER: DB entry is not used.\n");
        return SAI_STATUS_INVALID_OBJECT_ID;
    }

    status = stub_object_to_type(lag_db.members[lag_member_db_id].lag_oid,
                                 SAI_OBJECT_TYPE_LAG,
                                 &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG DB ID while removing LAG MEMBER.\n");
        return status;
    }

    if (lag_db.lags[lag_db_id].is_used) {
        for (ii = 0; ii < MAX_NUMBER_OF_LAG_MEMBERS; ii++) {
            if (lag_db.lags[lag_db_id].members_ids[ii] == lag_member_id) {
                lag_db.lags[lag_db_id].members_ids[ii] = 0;
                break;
            }
        }
    }

    memset(&lag_db.members[lag_member_db_id], 0, sizeof(lag_db.members[lag_member_db_id]));

    printf("REMOVE LAG_MEMBER: 0x%lX\n", lag_member_id);
    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_set_lag_member_attribute(
    _In_ sai_object_id_t  lag_member_id,
    _In_ const sai_attribute_t *attr)
{
    printf("SET LAG_MEMBER ATTRIBUTE is being called\n");
    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_get_lag_member_attribute(
    _In_ sai_object_id_t lag_member_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list)
{
    const sai_object_key_t key = { .object_id = lag_member_id };
    return sai_get_attributes(&key, NULL, lag_member_attribs, lag_member_vendor_attribs, attr_count, attr_list);
}

const sai_lag_api_t lag_api = {
    stub_create_lag,
    stub_remove_lag,
    stub_set_lag_attribute,
    stub_get_lag_attribute,
    stub_create_lag_member,
    stub_remove_lag_member,
    stub_set_lag_member_attribute,
    stub_get_lag_member_attribute
};
