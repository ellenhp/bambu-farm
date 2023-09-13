#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include "bambu_networking.hpp"
#include "ProjectTask.hpp"

#include <string>
#include <map>

using namespace BBL;
using namespace Slic3r;

#define EXPORT extern "C" __attribute__((visibility("default")))

EXPORT __attribute__((constructor)) void bambu_init();

__attribute__((visibility("default"))) void bambu_network_cb_printer_available(const std::string &device_id);
__attribute__((visibility("default"))) void bambu_network_cb_message_recv(const std::string &device_id, const std::string &message);
__attribute__((visibility("default"))) void bambu_network_cb_connected(const std::string &device_id);

EXPORT bool bambu_network_check_debug_consistent(bool is_debug);
EXPORT std::string bambu_network_get_version(void);
EXPORT void *bambu_network_create_agent(void);
EXPORT int bambu_network_destroy_agent(void *agent);
EXPORT int bambu_network_init_log(void *agent);
EXPORT int bambu_network_set_config_dir(void *agent, std::string config_dir);
EXPORT int bambu_network_set_cert_file(void *agent, std::string folder, std::string filename);
EXPORT int bambu_network_set_country_code(void *agent, std::string country_code);
EXPORT int bambu_network_start(void *agent);
EXPORT int bambu_network_set_on_ssdp_msg_fn(void *agent, OnMsgArrivedFn fn);
EXPORT int bambu_network_set_on_user_login_fn(void *agent, OnUserLoginFn fn);
EXPORT int bambu_network_set_on_printer_connected_fn(void *agent, OnPrinterConnectedFn fn);
EXPORT int bambu_network_set_on_server_connected_fn(void *agent, OnServerConnectedFn fn);
EXPORT int bambu_network_set_on_http_error_fn(void *agent, OnHttpErrorFn fn);
EXPORT int bambu_network_set_get_country_code_fn(void *agent, GetCountryCodeFn fn);
EXPORT int bambu_network_set_on_message_fn(void *agent, OnMessageFn fn);
EXPORT int bambu_network_set_on_local_connect_fn(void *agent, OnLocalConnectedFn fn);
EXPORT int bambu_network_set_on_local_message_fn(void *agent, OnMessageFn fn);
EXPORT int bambu_network_set_queue_on_main_fn(void *agent, QueueOnMainFn fn);
EXPORT int bambu_network_connect_server(void *agent);
EXPORT bool bambu_network_is_server_connected(void *agent);
EXPORT int bambu_network_refresh_connection(void *agent);
EXPORT int bambu_network_start_subscribe(void *agent, std::string module);
EXPORT int bambu_network_stop_subscribe(void *agent, std::string module);
EXPORT int bambu_network_send_message(void *agent, std::string dev_id, std::string json_str, int qos);
EXPORT int bambu_network_connect_printer(void *agent, std::string dev_id, std::string dev_ip, std::string username, std::string password, bool use_ssl);
EXPORT int bambu_network_disconnect_printer(void *agent);
EXPORT int bambu_network_send_message_to_printer(void *agent, std::string dev_id, std::string json_str, int qos);
EXPORT bool bambu_network_start_discovery(void *agent, bool start, bool sending);
EXPORT int bambu_network_change_user(void *agent, std::string user_info);
EXPORT bool bambu_network_is_user_login(void *agent);
EXPORT int bambu_network_user_logout(void *agent);
EXPORT std::string bambu_network_get_user_id(void *agent);
EXPORT std::string bambu_network_get_user_name(void *agent);
EXPORT std::string bambu_network_get_user_avatar(void *agent);
EXPORT std::string bambu_network_get_user_nickanme(void *agent);
EXPORT std::string bambu_network_build_login_cmd(void *agent);
EXPORT std::string bambu_network_build_logout_cmd(void *agent);
EXPORT std::string bambu_network_build_login_info(void *agent);
EXPORT int bambu_network_bind(void *agent, std::string dev_ip, std::string dev_id, std::string sec_link, std::string timezone, bool improved, OnUpdateStatusFn update_fn);
EXPORT int bambu_network_unbind(void *agent, std::string dev_id);
EXPORT std::string bambu_network_get_bambulab_host(void *agent);
EXPORT std::string bambu_network_get_user_selected_machine(void *agent);
EXPORT int bambu_network_set_user_selected_machine(void *agent, std::string dev_id);
EXPORT int bambu_network_start_print(void *agent, PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn);
EXPORT int bambu_network_start_local_print_with_record(void *agent, PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn);
EXPORT int bambu_network_start_send_gcode_to_sdcard(void *agent, PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn);
EXPORT int bambu_network_start_local_print(void *agent, PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn);
EXPORT int bambu_network_get_user_presets(void *agent, std::map<std::string, std::map<std::string, std::string>> *user_presets);
EXPORT std::string bambu_network_request_setting_id(void *agent, std::string name, std::map<std::string, std::string> *values_map, unsigned int *http_code);
EXPORT int bambu_network_put_setting(void *agent, std::string setting_id, std::string name, std::map<std::string, std::string> *values_map, unsigned int *http_code);
EXPORT int bambu_network_get_setting_list(void *agent, std::string bundle_version, ProgressFn pro_fn, WasCancelledFn cancel_fn);
EXPORT int bambu_network_delete_setting(void *agent, std::string setting_id);
EXPORT std::string bambu_network_get_studio_info_url(void *agent);
EXPORT int bambu_network_set_extra_http_header(void *agent, std::map<std::string, std::string> extra_headers);
EXPORT int bambu_network_get_my_message(void *agent, int type, int after, int limit, unsigned int *http_code, std::string *http_body);
EXPORT int bambu_network_check_user_task_report(void *agent, int *task_id, bool *printable);
EXPORT int bambu_network_get_user_print_info(void *agent, unsigned int *http_code, std::string *http_body);
EXPORT int bambu_network_get_printer_firmware(void *agent, std::string dev_id, unsigned *http_code, std::string *http_body);
EXPORT int bambu_network_get_task_plate_index(void *agent, std::string task_id, int *plate_index);
EXPORT int bambu_network_get_user_info(void *agent, int *identifier);
EXPORT int bambu_network_request_bind_ticket(void *agent, std::string *ticket);
EXPORT int bambu_network_get_slice_info(void *agent, std::string project_id, std::string profile_id, int plate_index, std::string *slice_json);
EXPORT int bambu_network_query_bind_status(void *agent, std::vector<std::string> query_list, unsigned int *http_code, std::string *http_body);
EXPORT int bambu_network_modify_printer_name(void *agent, std::string dev_id, std::string dev_name);
EXPORT int bambu_network_get_camera_url(void *agent, std::string dev_id, std::function<void(std::string)> callback);
EXPORT int bambu_network_get_design_staffpick(void *agent, int offset, int limit, std::function<void(std::string)> callback);
EXPORT int bambu_network_start_publish(void *agent, PublishParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, std::string *out);
EXPORT int bambu_network_get_profile_3mf(void *agent, BBLProfile *profile);
EXPORT int bambu_network_get_model_publish_url(void *agent, std::string *url);
EXPORT int bambu_network_get_subtask(void *agent, BBLModelTask *task);
EXPORT int bambu_network_get_model_mall_home_url(void *agent, std::string *url);
EXPORT int bambu_network_get_model_mall_detail_url(void *agent, std::string *url, std::string id);
EXPORT int bambu_network_get_my_profile(void *agent, std::string token, unsigned int *http_code, std::string *http_body);
EXPORT int bambu_network_track_enable(void *agent, bool enable);
EXPORT int bambu_network_track_event(void *agent, std::string evt_key, std::string content);
EXPORT int bambu_network_track_header(void *agent, std::string header);
EXPORT int bambu_network_track_update_property(void *agent, std::string name, std::string value, std::string type);
EXPORT int bambu_network_track_get_property(void *agent, std::string name, std::string &value, std::string type);

#endif // PLUGIN_API_H
