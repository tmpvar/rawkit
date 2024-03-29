#pragma once

#include <rawkit/jit.h>
#include <uv.h>

static void host_init_uv(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "uv_version", uv_version);
  rawkit_jit_add_export(jit, "uv_version_string", uv_version_string);
  rawkit_jit_add_export(jit, "uv_library_shutdown", uv_library_shutdown);
  rawkit_jit_add_export(jit, "uv_replace_allocator", uv_replace_allocator);
  rawkit_jit_add_export(jit, "uv_default_loop", uv_default_loop);
  rawkit_jit_add_export(jit, "uv_loop_init", uv_loop_init);
  rawkit_jit_add_export(jit, "uv_loop_close", uv_loop_close);
  rawkit_jit_add_export(jit, "uv_loop_new", uv_loop_new);
  rawkit_jit_add_export(jit, "uv_loop_delete", uv_loop_delete);
  rawkit_jit_add_export(jit, "uv_loop_size", uv_loop_size);
  rawkit_jit_add_export(jit, "uv_loop_alive", uv_loop_alive);
  rawkit_jit_add_export(jit, "uv_loop_configure", uv_loop_configure);
  rawkit_jit_add_export(jit, "uv_loop_fork", uv_loop_fork);
  rawkit_jit_add_export(jit, "uv_run", uv_run);
  rawkit_jit_add_export(jit, "uv_stop", uv_stop);
  rawkit_jit_add_export(jit, "uv_ref", uv_ref);
  rawkit_jit_add_export(jit, "uv_unref", uv_unref);
  rawkit_jit_add_export(jit, "uv_has_ref", uv_has_ref);
  rawkit_jit_add_export(jit, "uv_update_time", uv_update_time);
  rawkit_jit_add_export(jit, "uv_now", uv_now);
  rawkit_jit_add_export(jit, "uv_backend_fd", uv_backend_fd);
  rawkit_jit_add_export(jit, "uv_backend_timeout", uv_backend_timeout);
  rawkit_jit_add_export(jit, "uv_translate_sys_error", uv_translate_sys_error);
  rawkit_jit_add_export(jit, "uv_strerror", uv_strerror);
  rawkit_jit_add_export(jit, "uv_strerror_r", uv_strerror_r);
  rawkit_jit_add_export(jit, "uv_err_name", uv_err_name);
  rawkit_jit_add_export(jit, "uv_err_name_r", uv_err_name_r);
  rawkit_jit_add_export(jit, "uv_shutdown", uv_shutdown);
  rawkit_jit_add_export(jit, "uv_handle_size", uv_handle_size);
  rawkit_jit_add_export(jit, "uv_handle_get_type", uv_handle_get_type);
  rawkit_jit_add_export(jit, "uv_handle_type_name", uv_handle_type_name);
  rawkit_jit_add_export(jit, "uv_handle_get_data", uv_handle_get_data);
  rawkit_jit_add_export(jit, "uv_handle_get_loop", uv_handle_get_loop);
  rawkit_jit_add_export(jit, "uv_handle_set_data", uv_handle_set_data);
  rawkit_jit_add_export(jit, "uv_req_size", uv_req_size);
  rawkit_jit_add_export(jit, "uv_req_get_data", uv_req_get_data);
  rawkit_jit_add_export(jit, "uv_req_set_data", uv_req_set_data);
  rawkit_jit_add_export(jit, "uv_req_get_type", uv_req_get_type);
  rawkit_jit_add_export(jit, "uv_req_type_name", uv_req_type_name);
  rawkit_jit_add_export(jit, "uv_is_active", uv_is_active);
  rawkit_jit_add_export(jit, "uv_walk", uv_walk);
  rawkit_jit_add_export(jit, "uv_print_all_handles", uv_print_all_handles);
  rawkit_jit_add_export(jit, "uv_print_active_handles", uv_print_active_handles);
  rawkit_jit_add_export(jit, "uv_close", uv_close);
  rawkit_jit_add_export(jit, "uv_send_buffer_size", uv_send_buffer_size);
  rawkit_jit_add_export(jit, "uv_recv_buffer_size", uv_recv_buffer_size);
  rawkit_jit_add_export(jit, "uv_fileno", uv_fileno);
  rawkit_jit_add_export(jit, "uv_buf_init", uv_buf_init);
  rawkit_jit_add_export(jit, "uv_stream_get_write_queue_size", uv_stream_get_write_queue_size);
  rawkit_jit_add_export(jit, "uv_listen", uv_listen);
  rawkit_jit_add_export(jit, "uv_accept", uv_accept);
  rawkit_jit_add_export(jit, "uv_read_start", uv_read_start);
  rawkit_jit_add_export(jit, "uv_read_stop", uv_read_stop);
  rawkit_jit_add_export(jit, "uv_write", uv_write);
  rawkit_jit_add_export(jit, "uv_write2", uv_write2);
  rawkit_jit_add_export(jit, "uv_try_write", uv_try_write);
  rawkit_jit_add_export(jit, "uv_is_readable", uv_is_readable);
  rawkit_jit_add_export(jit, "uv_is_writable", uv_is_writable);
  rawkit_jit_add_export(jit, "uv_stream_set_blocking", uv_stream_set_blocking);
  rawkit_jit_add_export(jit, "uv_is_closing", uv_is_closing);
  rawkit_jit_add_export(jit, "uv_tcp_init", uv_tcp_init);
  rawkit_jit_add_export(jit, "uv_tcp_init_ex", uv_tcp_init_ex);
  rawkit_jit_add_export(jit, "uv_tcp_open", uv_tcp_open);
  rawkit_jit_add_export(jit, "uv_tcp_nodelay", uv_tcp_nodelay);
  rawkit_jit_add_export(jit, "uv_tcp_keepalive", uv_tcp_keepalive);
  rawkit_jit_add_export(jit, "uv_tcp_simultaneous_accepts", uv_tcp_simultaneous_accepts);
  rawkit_jit_add_export(jit, "uv_tcp_bind", uv_tcp_bind);
  rawkit_jit_add_export(jit, "uv_tcp_getsockname", uv_tcp_getsockname);
  rawkit_jit_add_export(jit, "uv_tcp_getpeername", uv_tcp_getpeername);
  rawkit_jit_add_export(jit, "uv_tcp_close_reset", uv_tcp_close_reset);
  rawkit_jit_add_export(jit, "uv_tcp_connect", uv_tcp_connect);
  rawkit_jit_add_export(jit, "uv_udp_init", uv_udp_init);
  rawkit_jit_add_export(jit, "uv_udp_init_ex", uv_udp_init_ex);
  rawkit_jit_add_export(jit, "uv_udp_open", uv_udp_open);
  rawkit_jit_add_export(jit, "uv_udp_bind", uv_udp_bind);
  rawkit_jit_add_export(jit, "uv_udp_connect", uv_udp_connect);
  rawkit_jit_add_export(jit, "uv_udp_getpeername", uv_udp_getpeername);
  rawkit_jit_add_export(jit, "uv_udp_getsockname", uv_udp_getsockname);
  rawkit_jit_add_export(jit, "uv_udp_set_membership", uv_udp_set_membership);
  rawkit_jit_add_export(jit, "uv_udp_set_source_membership", uv_udp_set_source_membership);
  rawkit_jit_add_export(jit, "uv_udp_set_multicast_loop", uv_udp_set_multicast_loop);
  rawkit_jit_add_export(jit, "uv_udp_set_multicast_ttl", uv_udp_set_multicast_ttl);
  rawkit_jit_add_export(jit, "uv_udp_set_multicast_interface", uv_udp_set_multicast_interface);
  rawkit_jit_add_export(jit, "uv_udp_set_broadcast", uv_udp_set_broadcast);
  rawkit_jit_add_export(jit, "uv_udp_set_ttl", uv_udp_set_ttl);
  rawkit_jit_add_export(jit, "uv_udp_send", uv_udp_send);
  rawkit_jit_add_export(jit, "uv_udp_try_send", uv_udp_try_send);
  rawkit_jit_add_export(jit, "uv_udp_recv_start", uv_udp_recv_start);
  rawkit_jit_add_export(jit, "uv_udp_using_recvmmsg", uv_udp_using_recvmmsg);
  rawkit_jit_add_export(jit, "uv_udp_recv_stop", uv_udp_recv_stop);
  rawkit_jit_add_export(jit, "uv_udp_get_send_queue_size", uv_udp_get_send_queue_size);
  rawkit_jit_add_export(jit, "uv_udp_get_send_queue_count", uv_udp_get_send_queue_count);
  rawkit_jit_add_export(jit, "uv_tty_init", uv_tty_init);
  rawkit_jit_add_export(jit, "uv_tty_set_mode", static_cast<int (*)(uv_tty_t*, uv_tty_mode_t mode)>(uv_tty_set_mode));
  rawkit_jit_add_export(jit, "uv_tty_reset_mode", uv_tty_reset_mode);
  rawkit_jit_add_export(jit, "uv_tty_get_winsize", uv_tty_get_winsize);
  rawkit_jit_add_export(jit, "uv_tty_set_vterm_state", uv_tty_set_vterm_state);
  rawkit_jit_add_export(jit, "uv_tty_get_vterm_state", uv_tty_get_vterm_state);
  rawkit_jit_add_export(jit, "uv_guess_handle", uv_guess_handle);
  rawkit_jit_add_export(jit, "uv_pipe_init", uv_pipe_init);
  rawkit_jit_add_export(jit, "uv_pipe_open", uv_pipe_open);
  rawkit_jit_add_export(jit, "uv_pipe_bind", uv_pipe_bind);
  rawkit_jit_add_export(jit, "uv_pipe_connect", uv_pipe_connect);
  rawkit_jit_add_export(jit, "uv_pipe_getsockname", uv_pipe_getsockname);
  rawkit_jit_add_export(jit, "uv_pipe_getpeername", uv_pipe_getpeername);
  rawkit_jit_add_export(jit, "uv_pipe_pending_instances", uv_pipe_pending_instances);
  rawkit_jit_add_export(jit, "uv_pipe_pending_count", uv_pipe_pending_count);
  rawkit_jit_add_export(jit, "uv_pipe_pending_type", uv_pipe_pending_type);
  rawkit_jit_add_export(jit, "uv_pipe_chmod", uv_pipe_chmod);
  rawkit_jit_add_export(jit, "uv_poll_init", uv_poll_init);
  rawkit_jit_add_export(jit, "uv_poll_init_socket", uv_poll_init_socket);
  rawkit_jit_add_export(jit, "uv_poll_start", uv_poll_start);
  rawkit_jit_add_export(jit, "uv_poll_stop", uv_poll_stop);
  rawkit_jit_add_export(jit, "uv_prepare_init", uv_prepare_init);
  rawkit_jit_add_export(jit, "uv_prepare_start", uv_prepare_start);
  rawkit_jit_add_export(jit, "uv_prepare_stop", uv_prepare_stop);
  rawkit_jit_add_export(jit, "uv_check_init", uv_check_init);
  rawkit_jit_add_export(jit, "uv_check_start", uv_check_start);
  rawkit_jit_add_export(jit, "uv_check_stop", uv_check_stop);
  rawkit_jit_add_export(jit, "uv_idle_init", uv_idle_init);
  rawkit_jit_add_export(jit, "uv_idle_start", uv_idle_start);
  rawkit_jit_add_export(jit, "uv_idle_stop", uv_idle_stop);
  rawkit_jit_add_export(jit, "uv_async_init", uv_async_init);
  rawkit_jit_add_export(jit, "uv_async_send", uv_async_send);
  rawkit_jit_add_export(jit, "uv_timer_init", uv_timer_init);
  rawkit_jit_add_export(jit, "uv_timer_start", uv_timer_start);
  rawkit_jit_add_export(jit, "uv_timer_stop", uv_timer_stop);
  rawkit_jit_add_export(jit, "uv_timer_again", uv_timer_again);
  rawkit_jit_add_export(jit, "uv_timer_set_repeat", uv_timer_set_repeat);
  rawkit_jit_add_export(jit, "uv_timer_get_repeat", uv_timer_get_repeat);
  rawkit_jit_add_export(jit, "uv_getaddrinfo", uv_getaddrinfo);
  rawkit_jit_add_export(jit, "uv_freeaddrinfo", uv_freeaddrinfo);
  rawkit_jit_add_export(jit, "uv_getnameinfo", uv_getnameinfo);
  rawkit_jit_add_export(jit, "uv_spawn", uv_spawn);
  rawkit_jit_add_export(jit, "uv_process_kill", uv_process_kill);
  rawkit_jit_add_export(jit, "uv_kill", uv_kill);
  rawkit_jit_add_export(jit, "uv_process_get_pid", uv_process_get_pid);
  rawkit_jit_add_export(jit, "uv_queue_work", uv_queue_work);
  rawkit_jit_add_export(jit, "uv_cancel", uv_cancel);
  rawkit_jit_add_export(jit, "uv_setup_args", uv_setup_args);
  rawkit_jit_add_export(jit, "uv_get_process_title", uv_get_process_title);
  rawkit_jit_add_export(jit, "uv_set_process_title", uv_set_process_title);
  rawkit_jit_add_export(jit, "uv_resident_set_memory", uv_resident_set_memory);
  rawkit_jit_add_export(jit, "uv_uptime", uv_uptime);
  rawkit_jit_add_export(jit, "uv_get_osfhandle", uv_get_osfhandle);
  rawkit_jit_add_export(jit, "uv_open_osfhandle", uv_open_osfhandle);
  rawkit_jit_add_export(jit, "uv_getrusage", uv_getrusage);
  rawkit_jit_add_export(jit, "uv_os_homedir", uv_os_homedir);
  rawkit_jit_add_export(jit, "uv_os_tmpdir", uv_os_tmpdir);
  rawkit_jit_add_export(jit, "uv_os_get_passwd", uv_os_get_passwd);
  rawkit_jit_add_export(jit, "uv_os_free_passwd", uv_os_free_passwd);
  rawkit_jit_add_export(jit, "uv_os_getpid", uv_os_getpid);
  rawkit_jit_add_export(jit, "uv_os_getppid", uv_os_getppid);
  rawkit_jit_add_export(jit, "uv_os_getpriority", uv_os_getpriority);
  rawkit_jit_add_export(jit, "uv_os_setpriority", uv_os_setpriority);
  rawkit_jit_add_export(jit, "uv_cpu_info", uv_cpu_info);
  rawkit_jit_add_export(jit, "uv_free_cpu_info", uv_free_cpu_info);
  rawkit_jit_add_export(jit, "uv_interface_addresses", uv_interface_addresses);
  rawkit_jit_add_export(jit, "uv_free_interface_addresses", uv_free_interface_addresses);
  rawkit_jit_add_export(jit, "uv_os_environ", uv_os_environ);
  rawkit_jit_add_export(jit, "uv_os_free_environ", uv_os_free_environ);
  rawkit_jit_add_export(jit, "uv_os_getenv", uv_os_getenv);
  rawkit_jit_add_export(jit, "uv_os_setenv", uv_os_setenv);
  rawkit_jit_add_export(jit, "uv_os_unsetenv", uv_os_unsetenv);
  rawkit_jit_add_export(jit, "uv_os_gethostname", uv_os_gethostname);
  rawkit_jit_add_export(jit, "uv_os_uname", uv_os_uname);
  rawkit_jit_add_export(jit, "uv_metrics_idle_time", uv_metrics_idle_time);
  rawkit_jit_add_export(jit, "uv_fs_get_type", uv_fs_get_type);
  rawkit_jit_add_export(jit, "uv_fs_get_result", uv_fs_get_result);
  rawkit_jit_add_export(jit, "uv_fs_get_system_error", uv_fs_get_system_error);
  rawkit_jit_add_export(jit, "uv_fs_get_ptr", uv_fs_get_ptr);
  rawkit_jit_add_export(jit, "uv_fs_get_path", uv_fs_get_path);
  rawkit_jit_add_export(jit, "uv_fs_get_statbuf", uv_fs_get_statbuf);
  rawkit_jit_add_export(jit, "uv_fs_req_cleanup", uv_fs_req_cleanup);
  rawkit_jit_add_export(jit, "uv_fs_close", uv_fs_close);
  rawkit_jit_add_export(jit, "uv_fs_open", uv_fs_open);
  rawkit_jit_add_export(jit, "uv_fs_read", uv_fs_read);
  rawkit_jit_add_export(jit, "uv_fs_unlink", uv_fs_unlink);
  rawkit_jit_add_export(jit, "uv_fs_write", uv_fs_write);
  rawkit_jit_add_export(jit, "uv_fs_copyfile", uv_fs_copyfile);
  rawkit_jit_add_export(jit, "uv_fs_mkdir", uv_fs_mkdir);
  rawkit_jit_add_export(jit, "uv_fs_mkdtemp", uv_fs_mkdtemp);
  rawkit_jit_add_export(jit, "uv_fs_mkstemp", uv_fs_mkstemp);
  rawkit_jit_add_export(jit, "uv_fs_rmdir", uv_fs_rmdir);
  rawkit_jit_add_export(jit, "uv_fs_scandir", uv_fs_scandir);
  rawkit_jit_add_export(jit, "uv_fs_scandir_next", uv_fs_scandir_next);
  rawkit_jit_add_export(jit, "uv_fs_opendir", uv_fs_opendir);
  rawkit_jit_add_export(jit, "uv_fs_readdir", uv_fs_readdir);
  rawkit_jit_add_export(jit, "uv_fs_closedir", uv_fs_closedir);
  rawkit_jit_add_export(jit, "uv_fs_stat", uv_fs_stat);
  rawkit_jit_add_export(jit, "uv_fs_fstat", uv_fs_fstat);
  rawkit_jit_add_export(jit, "uv_fs_rename", uv_fs_rename);
  rawkit_jit_add_export(jit, "uv_fs_fsync", uv_fs_fsync);
  rawkit_jit_add_export(jit, "uv_fs_fdatasync", uv_fs_fdatasync);
  rawkit_jit_add_export(jit, "uv_fs_ftruncate", uv_fs_ftruncate);
  rawkit_jit_add_export(jit, "uv_fs_sendfile", uv_fs_sendfile);
  rawkit_jit_add_export(jit, "uv_fs_access", uv_fs_access);
  rawkit_jit_add_export(jit, "uv_fs_chmod", uv_fs_chmod);
  rawkit_jit_add_export(jit, "uv_fs_utime", uv_fs_utime);
  rawkit_jit_add_export(jit, "uv_fs_futime", uv_fs_futime);
  rawkit_jit_add_export(jit, "uv_fs_lutime", uv_fs_lutime);
  rawkit_jit_add_export(jit, "uv_fs_lstat", uv_fs_lstat);
  rawkit_jit_add_export(jit, "uv_fs_link", uv_fs_link);
  rawkit_jit_add_export(jit, "uv_fs_symlink", uv_fs_symlink);
  rawkit_jit_add_export(jit, "uv_fs_readlink", uv_fs_readlink);
  rawkit_jit_add_export(jit, "uv_fs_realpath", uv_fs_realpath);
  rawkit_jit_add_export(jit, "uv_fs_fchmod", uv_fs_fchmod);
  rawkit_jit_add_export(jit, "uv_fs_chown", uv_fs_chown);
  rawkit_jit_add_export(jit, "uv_fs_fchown", uv_fs_fchown);
  rawkit_jit_add_export(jit, "uv_fs_lchown", uv_fs_lchown);
  rawkit_jit_add_export(jit, "uv_fs_statfs", uv_fs_statfs);
  rawkit_jit_add_export(jit, "uv_fs_poll_init", uv_fs_poll_init);
  rawkit_jit_add_export(jit, "uv_fs_poll_start", uv_fs_poll_start);
  rawkit_jit_add_export(jit, "uv_fs_poll_stop", uv_fs_poll_stop);
  rawkit_jit_add_export(jit, "uv_fs_poll_getpath", uv_fs_poll_getpath);
  rawkit_jit_add_export(jit, "uv_signal_init", uv_signal_init);
  rawkit_jit_add_export(jit, "uv_signal_start", uv_signal_start);
  rawkit_jit_add_export(jit, "uv_signal_start_oneshot", uv_signal_start_oneshot);
  rawkit_jit_add_export(jit, "uv_signal_stop", uv_signal_stop);
  rawkit_jit_add_export(jit, "uv_loadavg", uv_loadavg);
  rawkit_jit_add_export(jit, "uv_fs_event_init", uv_fs_event_init);
  rawkit_jit_add_export(jit, "uv_fs_event_start", uv_fs_event_start);
  rawkit_jit_add_export(jit, "uv_fs_event_stop", uv_fs_event_stop);
  rawkit_jit_add_export(jit, "uv_fs_event_getpath", uv_fs_event_getpath);
  rawkit_jit_add_export(jit, "uv_ip4_addr", uv_ip4_addr);
  rawkit_jit_add_export(jit, "uv_ip6_addr", uv_ip6_addr);
  rawkit_jit_add_export(jit, "uv_ip4_name", uv_ip4_name);
  rawkit_jit_add_export(jit, "uv_ip6_name", uv_ip6_name);
  rawkit_jit_add_export(jit, "uv_inet_ntop", uv_inet_ntop);
  rawkit_jit_add_export(jit, "uv_inet_pton", uv_inet_pton);
  rawkit_jit_add_export(jit, "uv_random", uv_random);
  rawkit_jit_add_export(jit, "uv_if_indextoname", uv_if_indextoname);
  rawkit_jit_add_export(jit, "uv_if_indextoiid", uv_if_indextoiid);
  rawkit_jit_add_export(jit, "uv_exepath", uv_exepath);
  rawkit_jit_add_export(jit, "uv_cwd", uv_cwd);
  rawkit_jit_add_export(jit, "uv_chdir", uv_chdir);
  rawkit_jit_add_export(jit, "uv_get_free_memory", uv_get_free_memory);
  rawkit_jit_add_export(jit, "uv_get_total_memory", uv_get_total_memory);
  rawkit_jit_add_export(jit, "uv_get_constrained_memory", uv_get_constrained_memory);
  rawkit_jit_add_export(jit, "uv_hrtime", uv_hrtime);
  rawkit_jit_add_export(jit, "uv_sleep", uv_sleep);
  rawkit_jit_add_export(jit, "uv_disable_stdio_inheritance", uv_disable_stdio_inheritance);
  rawkit_jit_add_export(jit, "uv_dlopen", uv_dlopen);
  rawkit_jit_add_export(jit, "uv_dlclose", uv_dlclose);
  rawkit_jit_add_export(jit, "uv_dlsym", uv_dlsym);
  rawkit_jit_add_export(jit, "uv_dlerror", uv_dlerror);
  rawkit_jit_add_export(jit, "uv_mutex_init", uv_mutex_init);
  rawkit_jit_add_export(jit, "uv_mutex_init_recursive", uv_mutex_init_recursive);
  rawkit_jit_add_export(jit, "uv_mutex_destroy", uv_mutex_destroy);
  rawkit_jit_add_export(jit, "uv_mutex_lock", uv_mutex_lock);
  rawkit_jit_add_export(jit, "uv_mutex_trylock", uv_mutex_trylock);
  rawkit_jit_add_export(jit, "uv_mutex_unlock", uv_mutex_unlock);
  rawkit_jit_add_export(jit, "uv_rwlock_init", uv_rwlock_init);
  rawkit_jit_add_export(jit, "uv_rwlock_destroy", uv_rwlock_destroy);
  rawkit_jit_add_export(jit, "uv_rwlock_rdlock", uv_rwlock_rdlock);
  rawkit_jit_add_export(jit, "uv_rwlock_tryrdlock", uv_rwlock_tryrdlock);
  rawkit_jit_add_export(jit, "uv_rwlock_rdunlock", uv_rwlock_rdunlock);
  rawkit_jit_add_export(jit, "uv_rwlock_wrlock", uv_rwlock_wrlock);
  rawkit_jit_add_export(jit, "uv_rwlock_trywrlock", uv_rwlock_trywrlock);
  rawkit_jit_add_export(jit, "uv_rwlock_wrunlock", uv_rwlock_wrunlock);
  rawkit_jit_add_export(jit, "uv_sem_init", uv_sem_init);
  rawkit_jit_add_export(jit, "uv_sem_destroy", uv_sem_destroy);
  rawkit_jit_add_export(jit, "uv_sem_post", uv_sem_post);
  rawkit_jit_add_export(jit, "uv_sem_wait", uv_sem_wait);
  rawkit_jit_add_export(jit, "uv_sem_trywait", uv_sem_trywait);
  rawkit_jit_add_export(jit, "uv_cond_init", uv_cond_init);
  rawkit_jit_add_export(jit, "uv_cond_destroy", uv_cond_destroy);
  rawkit_jit_add_export(jit, "uv_cond_signal", uv_cond_signal);
  rawkit_jit_add_export(jit, "uv_cond_broadcast", uv_cond_broadcast);
  rawkit_jit_add_export(jit, "uv_barrier_init", uv_barrier_init);
  rawkit_jit_add_export(jit, "uv_barrier_destroy", uv_barrier_destroy);
  rawkit_jit_add_export(jit, "uv_barrier_wait", uv_barrier_wait);
  rawkit_jit_add_export(jit, "uv_cond_wait", uv_cond_wait);
  rawkit_jit_add_export(jit, "uv_cond_timedwait", uv_cond_timedwait);
  rawkit_jit_add_export(jit, "uv_once", uv_once);
  rawkit_jit_add_export(jit, "uv_key_create", uv_key_create);
  rawkit_jit_add_export(jit, "uv_key_delete", uv_key_delete);
  rawkit_jit_add_export(jit, "uv_key_get", uv_key_get);
  rawkit_jit_add_export(jit, "uv_key_set", uv_key_set);
  rawkit_jit_add_export(jit, "uv_gettimeofday", uv_gettimeofday);
  rawkit_jit_add_export(jit, "uv_thread_create", uv_thread_create);
  rawkit_jit_add_export(jit, "uv_thread_create_ex", uv_thread_create_ex);
  rawkit_jit_add_export(jit, "uv_thread_self", uv_thread_self);
  rawkit_jit_add_export(jit, "uv_thread_join", uv_thread_join);
  rawkit_jit_add_export(jit, "uv_thread_equal", uv_thread_equal);
  rawkit_jit_add_export(jit, "uv_loop_get_data", uv_loop_get_data);
  rawkit_jit_add_export(jit, "uv_loop_set_data", uv_loop_set_data);
}

