#include "api.hpp"
#include "stdio.h"

#include <chrono>
#include <thread>

#include "bambu-farm-client/src/api.rs.h"

#define LOG_CALL_ARGS(FORMAT, ...) {\
    char _log_macro_buf[1024];\
    snprintf(_log_macro_buf, 1024, "%s (Line %d): " FORMAT "\n", __FUNCTION__, __LINE__, __VA_ARGS__);\
    bambu_network_rs_log_debug(std::string(_log_macro_buf));\
    }
#define LOG_CALL() {\
    char _log_macro_buf[1024];\
    snprintf(_log_macro_buf, 1024, "%s (Line %d)\n", __FUNCTION__, __LINE__);\
    bambu_network_rs_log_debug(std::string(_log_macro_buf));\
    }

static OnMsgArrivedFn on_msg_arrived;
static OnLocalConnectedFn on_local_connect;
static OnMessageFn on_mqtt_message;
static std::string selected_device;
static std::string ca_path;

void bambu_init() {
    LOG_CALL();
}

void bambu_network_cb_printer_available(const std::string &json) {
    if (on_msg_arrived) {
        on_msg_arrived(json);
    }
}
void bambu_network_cb_message_recv(const std::string &device_id, const std::string &message) {
    if (on_mqtt_message) {
        LOG_CALL_ARGS("Calling MQTT callback with:\n%s", message.c_str());
        on_mqtt_message(device_id, message);
    }
}
void bambu_network_cb_connected(const std::string &device_id) {
    LOG_CALL();
    if (on_local_connect) {
        LOG_CALL_ARGS("Calling connected callback for device %s", device_id.c_str());
        on_local_connect(0, device_id, "Connected");
    }
}

bool bambu_network_check_debug_consistent(bool is_debug)
{
    LOG_CALL();
    return true;
}
std::string bambu_network_get_version(void)
{
    LOG_CALL();
    return BAMBU_NETWORK_AGENT_VERSION;
}
void *bambu_network_create_agent(void)
{
    LOG_CALL();
    bambu_network_rs_init();
    return (void*)0x10000000;
}
int bambu_network_destroy_agent(void *agent_ptr)
{
    LOG_CALL();
    return 0;
}
int bambu_network_init_log(void *agent_ptr)
{
    LOG_CALL();
    return 0;
}
int bambu_network_set_config_dir(void *agent_ptr, std::string config_dir)
{
    LOG_CALL_ARGS("%s", config_dir.c_str());
    return 0;
}
int bambu_network_set_cert_file(void *agent_ptr, std::string folder, std::string filename)
{
    LOG_CALL();
    ca_path = folder + "/" + filename;
    return 0;
}
int bambu_network_set_country_code(void *agent_ptr, std::string country_code)
{
    LOG_CALL();
    // No region-locked features.
    return 0;
}
int bambu_network_start(void *agent_ptr)
{
    LOG_CALL();
    return 0;
}
int bambu_network_set_on_ssdp_msg_fn(void *agent_ptr, OnMsgArrivedFn fn)
{
    LOG_CALL();
    on_msg_arrived = fn;
    return 0;
}
int bambu_network_set_on_user_login_fn(void *agent_ptr, OnUserLoginFn fn)
{
    LOG_CALL();
    return 0;
}
int bambu_network_set_on_printer_connected_fn(void *agent_ptr, OnPrinterConnectedFn fn)
{
    LOG_CALL();
    return 0;
}
int bambu_network_set_on_server_connected_fn(void *agent_ptr, OnServerConnectedFn fn)
{
    LOG_CALL();
    return 0;
}
int bambu_network_set_on_http_error_fn(void *agent_ptr, OnHttpErrorFn fn)
{
    LOG_CALL();
    return 0;
}
int bambu_network_set_get_country_code_fn(void *agent_ptr, GetCountryCodeFn fn)
{
    LOG_CALL();
    // No region-locked features.
    return 0;
}
int bambu_network_set_on_message_fn(void *agent_ptr, OnMessageFn fn)
{
    LOG_CALL();
    // TBD but it seems like this only applies to server connections.
    return 0;
}
int bambu_network_set_on_local_connect_fn(void *agent_ptr, OnLocalConnectedFn fn)
{
    LOG_CALL();
    on_local_connect = fn;
    return 0;
}
int bambu_network_set_on_local_message_fn(void *agent_ptr, OnMessageFn fn)
{
    LOG_CALL();
    on_mqtt_message = fn;
    return 0;
}
int bambu_network_set_queue_on_main_fn(void *agent_ptr, QueueOnMainFn fn)
{
    LOG_CALL();
    return 0;
}
int bambu_network_connect_server(void *agent_ptr)
{
    LOG_CALL();
    // No-op.
    return 0; // Maybe BAMBU_NETWORK_ERR_CONNECT_FAILED is more appropriate?
}
bool bambu_network_is_server_connected(void *agent_ptr)
{
    LOG_CALL();
    return false;
}
int bambu_network_refresh_connection(void *agent_ptr)
{
    LOG_CALL();
    return 0; // Maybe BAMBU_NETWORK_ERR_CONNECT_FAILED is more appropriate?
}
int bambu_network_start_subscribe(void *agent_ptr, std::string module)
{
    LOG_CALL();
    return 0; // Maybe BAMBU_NETWORK_ERR_CONNECT_FAILED is more appropriate?
}
int bambu_network_stop_subscribe(void *agent_ptr, std::string module)
{
    LOG_CALL();
    return 0;
}
int bambu_network_send_message(void *agent_ptr, std::string dev_id, std::string json_str, int qos)
{
    LOG_CALL();
    // TODO: Maybe send message here, but this seems cloud-only.
    return 0;
}
int bambu_network_connect_printer(void *agent_ptr, std::string dev_id, std::string dev_ip, std::string username, std::string password, bool use_ssl)
{
    LOG_CALL_ARGS("%s %s %s %s", dev_id.c_str(), dev_ip.c_str(), username.c_str(), password.c_str());
    bambu_network_rs_connect(dev_id);
    return 0;
}
int bambu_network_disconnect_printer(void *agent_ptr)
{
    LOG_CALL();
    return 0;
}
int bambu_network_send_message_to_printer(void *agent_ptr, std::string dev_id, std::string json_str, int qos)
{
    LOG_CALL_ARGS("%s %s", dev_id.c_str(), json_str.c_str());
    bambu_network_rs_send(dev_id, json_str);
    return 0;
}
bool bambu_network_start_discovery(void *agent_ptr, bool start, bool sending)
{
    LOG_CALL();
    return 0;
}
int bambu_network_change_user(void *agent_ptr, std::string user_info)
{
    return BAMBU_NETWORK_ERR_CONNECT_FAILED;
}
bool bambu_network_is_user_login(void *agent_ptr)
{
    return false;
}
int bambu_network_user_logout(void *agent_ptr)
{
    return 0;
}
std::string bambu_network_get_user_id(void *agent_ptr)
{
    return "";
}
std::string bambu_network_get_user_name(void *agent_ptr)
{
    return "";
}
std::string bambu_network_get_user_avatar(void *agent_ptr)
{
    return "";
}
std::string bambu_network_get_user_nickanme(void *agent_ptr)
{
    return "";
}
std::string bambu_network_build_login_cmd(void *agent_ptr)
{
    return "";
}
std::string bambu_network_build_logout_cmd(void *agent_ptr)
{
    return "";
}
std::string bambu_network_build_login_info(void *agent_ptr)
{
    return "";
}
int bambu_network_bind(void *agent_ptr, std::string dev_ip, std::string dev_id, std::string sec_link, std::string timezone, bool improved, OnUpdateStatusFn update_fn)
{
    LOG_CALL();
    return 0;
}
int bambu_network_unbind(void *agent_ptr, std::string dev_id)
{
    LOG_CALL();
    return 0;
}
std::string bambu_network_get_bambulab_host(void *agent_ptr)
{
    LOG_CALL();
    return "localhost";
}
std::string bambu_network_get_user_selected_machine(void *agent_ptr)
{
    LOG_CALL();
    return selected_device;
}
int bambu_network_set_user_selected_machine(void *agent_ptr, std::string dev_id)
{
    LOG_CALL_ARGS("%s", dev_id.c_str());
    selected_device = dev_id;
    return 0;
}
int bambu_network_start_print(void *agent_ptr, PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn)
{
    LOG_CALL();
    return bambu_network_start_local_print(agent_ptr, params, update_fn, cancel_fn);
}
int bambu_network_start_local_print_with_record(void *agent_ptr, PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn)
{
    LOG_CALL();
    return bambu_network_start_local_print(agent_ptr, params, update_fn, cancel_fn);
}
int bambu_network_start_send_gcode_to_sdcard(void *agent_ptr, PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn)
{
    LOG_CALL_ARGS("%s %s %s %s", params.project_name.c_str(), params.filename.c_str(), params.ftp_file.c_str(), params.dst_file.c_str());
    return 0;
}
int bambu_network_start_local_print(void *agent_ptr, PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn)
{
    LOG_CALL_ARGS("%s %s %s", params.project_name.c_str(), params.filename.c_str(), params.ams_mapping.c_str());
    bambu_network_rs_upload_file(params.dev_id, params.filename, "print.gcode.3mf");
    std::this_thread::sleep_for (std::chrono::seconds(1));
    std::string json = "{\"print\":{\"sequence_id\":0,\"command\":\"project_file\",\"param\":\"Metadata/plate_1.gcode\",\"subtask_name\":\"print.gcode.3mf\",\"url\":\"ftp://print.gcode.3mf\",\"timelapse\":false,\"bed_leveling\":true,\"flow_cali\":false,\"vibration_cali\":false,\"layer_inspect\":true,\"use_ams\":true}}";
    bambu_network_rs_send(params.dev_id, json);
    return 0;
}
int bambu_network_get_user_presets(void *agent_ptr, std::map<std::string, std::map<std::string, std::string>> *user_presets)
{
    LOG_CALL();
    return 0;
}
std::string bambu_network_request_setting_id(void *agent_ptr, std::string name, std::map<std::string, std::string> *values_map, unsigned int *http_code)
{
    LOG_CALL();
    return 0;
}
int bambu_network_put_setting(void *agent_ptr, std::string setting_id, std::string name, std::map<std::string, std::string> *values_map, unsigned int *http_code)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_setting_list(void *agent_ptr, std::string bundle_version, ProgressFn pro_fn, WasCancelledFn cancel_fn)
{
    LOG_CALL();
    return 0;
}
int bambu_network_delete_setting(void *agent_ptr, std::string setting_id)
{
    LOG_CALL();
    return 0;
}
std::string bambu_network_get_studio_info_url(void *agent_ptr)
{
    LOG_CALL();
    return "";
}
int bambu_network_set_extra_http_header(void *agent_ptr, std::map<std::string, std::string> extra_headers)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_my_message(void *agent_ptr, int type, int after, int limit, unsigned int *http_code, std::string *http_body)
{
    LOG_CALL();
    return 0;
}
int bambu_network_check_user_task_report(void *agent_ptr, int *task_id, bool *printable)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_user_print_info(void *agent_ptr, unsigned int *http_code, std::string *http_body)
{
    LOG_CALL();
    return 0;
}

int bambu_network_get_printer_firmware(void *agent_ptr, std::string dev_id, unsigned *http_code, std::string *http_body)
{
    LOG_CALL();
    *http_code = 500;
    *http_body = "";
    return 0;
}
int bambu_network_get_task_plate_index(void *agent_ptr, std::string task_id, int *plate_index)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_user_info(void *agent_ptr, int *identifier)
{
    LOG_CALL();
    return 0;
}
int bambu_network_request_bind_ticket(void *agent_ptr, std::string *ticket)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_slice_info(void *agent_ptr, std::string project_id, std::string profile_id, int plate_index, std::string *slice_json)
{
    LOG_CALL();
    return 0;
}
int bambu_network_query_bind_status(void *agent_ptr, std::vector<std::string> query_list, unsigned int *http_code, std::string *http_body)
{
    LOG_CALL();
    return 0;
}
int bambu_network_modify_printer_name(void *agent_ptr, std::string dev_id, std::string dev_name)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_camera_url(void *agent_ptr, std::string dev_id, std::function<void(std::string)> callback)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_design_staffpick(void *agent_ptr, int offset, int limit, std::function<void(std::string)> callback)
{
    LOG_CALL();
    return 0;
}
int bambu_network_start_publish(void *agent_ptr, PublishParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, std::string *out)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_profile_3mf(void *agent_ptr, BBLProfile *profile)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_model_publish_url(void *agent_ptr, std::string *url)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_subtask(void *agent_ptr, BBLModelTask *task)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_model_mall_home_url(void *agent_ptr, std::string *url)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_model_mall_detail_url(void *agent_ptr, std::string *url, std::string id)
{
    LOG_CALL();
    return 0;
}
int bambu_network_get_my_profile(void *agent_ptr, std::string token, unsigned int *http_code, std::string *http_body)
{
    LOG_CALL();
    return 0;
}
int bambu_network_track_enable(void *agent_ptr, bool enable)
{
    LOG_CALL();
    return 0;
}
int bambu_network_track_event(void *agent_ptr, std::string evt_key, std::string content)
{
    LOG_CALL();
    return 0;
}
int bambu_network_track_header(void *agent_ptr, std::string header)
{
    LOG_CALL();
    return 0;
}
int bambu_network_track_update_property(void *agent_ptr, std::string name, std::string value, std::string type)
{
    LOG_CALL();
    return 0;
}
int bambu_network_track_get_property(void *agent_ptr, std::string name, std::string &value, std::string type)
{
    LOG_CALL();
    value = "";
    return 0;
}
