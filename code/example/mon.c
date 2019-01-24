#include "mon.h"
#include <common.h>
#include "zlog_common.h"
#include "mon_cf_parse.h"
#include "mon_cmdline.h"
#include "mon_dbg.h"
#include "cf_ops.h"
#include <sys/prctl.h>

#ifdef	USE_MQTT
#include "mosquitto.h"
#include "broker.h"
#else
#include "param.h"
#include "client.h"
#endif


#define	MON_LOG_CONF_FILE_DEFAULT	"/mnt/env/zlog_default.conf"

#define	MON_LOG_CAT_DEFAULT		"default_cat"

struct global_arg_s g_args;

char err_jsonstr[] = "{\"stat\":\"-1\"}";

log_category_t *current_logcat = NULL;

int process_mon(void *args, int options);

#ifdef	USE_MQTT
int mqtt_disconnect_process(void* arg)
{
	
}

#endif

static int global_args_init(struct global_arg_s *args)
{
	int ret = 0;

	args->dbg = 0;
	args->prog_name = NULL;
	args->conf_file = NULL;
	args->log_file = NULL;
	args->print_read_val = 0;
	args->reload_cf = 1;
	args->st_m = ST_M_INIT;
	args->retry = DEFAULT_RETRY_TIME;

	INIT_LIST_HEAD(&(args->excel_tb));
	INIT_LIST_HEAD(&(args->run_tb));

	args->child.running = 0;

	pthread_mutex_init(&(args->child.mutex), NULL);
	pthread_cond_init(&(args->child.cond), NULL);

	pthread_mutex_init(&(args->mon.mutex), NULL);
	pthread_cond_init(&(args->mon.cond), NULL);
	
	pthread_mutex_init(&(args->arg_mutex), NULL);
	pthread_cond_init(&(args->arg_cond), NULL);

	return ret;
}

static void global_args_clean(struct global_arg_s *args)
{
	//clean_inittab(args);
	pthread_cond_destroy(&(args->child.cond));
	pthread_cond_destroy(&(args->mon.cond));
	pthread_cond_destroy(&(args->arg_cond));

	pthread_mutex_destroy(&(args->child.mutex));
	pthread_mutex_destroy(&(args->mon.mutex));
	pthread_mutex_destroy(&(args->arg_mutex));
}

int mon_log_init(const char *conf, const char *cat)
{
	int ret = 0;
	if(conf){
		ret = log_init(conf);
	}else{
		ret = log_init(MON_LOG_CONF_FILE_DEFAULT);
	}

	if (ret < 0) {
		fprintf (stderr, "log_init fail(ret = %d)\n",ret);
		goto exit;
	}
	current_logcat = log_get_category(cat);
	if (NULL == current_logcat) {
		fprintf (stderr, "get default cat.\n");
		current_logcat = log_get_category(MON_LOG_CAT_DEFAULT);
		if (NULL == current_logcat) {
			fprintf (stderr, "log_get_category fail\n");
			ret = -EINVAL;
			goto exit;
		}
	}
exit:
	return ret;
}

/* Wrapper around exec:
 * Takes string.
 * If chars like '>' detected, execs '[-]/bin/sh -c "exec ......."'.
 * Otherwise splits words on whitespace, deals with leading dash,
 * and uses plain exec().
 * NB: careful, we can be called after vfork!
 */
static int init_exec(const char *command)
{
	/* +8 allows to write VLA sizes below more efficiently: */
	unsigned command_size = strlen(command) + 8;
	/* strlen(command) + strlen("exec ")+1: */
	char buf[command_size];
	/* strlen(command) / 2 + 4: */
	char *cmd[command_size / 2];
	int dash;

	dash = (command[0] == '-' /* maybe? && command[1] == '/' */);
	command += dash;

	/* See if any special /bin/sh requiring characters are present */
	if (strpbrk(command, "~`!$^&*()=|\\{}[];\"'<>?") != NULL) {
		sprintf(buf, "exec %s", command); /* excluding "-" */
		/* NB: LIBBB_DEFAULT_LOGIN_SHELL define has leading dash */
		cmd[0] = (char*)(DEFAULT_LOGIN_SHELL + !dash);
		cmd[1] = (char*)"-c";
		cmd[2] = buf;
		cmd[3] = NULL;
		command = DEFAULT_LOGIN_SHELL + 1;
	} else {
		/* Convert command (char*) into cmd (char**, one word per string) */
		char *word, *next;
		int i = 0;
		next = strcpy(buf, command - dash); /* command including "-" */
		command = next + dash;
		while ((word = strsep(&next, " \t")) != NULL) {
			if (*word != '\0') { /* not two spaces/tabs together? */
				cmd[i] = word;
				i++;
			}
		}
		cmd[i] = NULL;
	}
#if 0
	/* If we saw leading "-", it is interactive shell.
	 * Try harder to give it a controlling tty.
	 */
	if (ENABLE_FEATURE_INIT_SCTTY && dash) {
		/* _Attempt_ to make stdin a controlling tty. */
		ioctl(STDIN_FILENO, TIOCSCTTY, 0 /*only try, don't steal*/);
	}
#endif
	/* Here command never contains the dash, cmd[0] might */
	execvp(command, cmd);

	log_error("can't run '%s': "STRERROR_FMT, command STRERROR_ERRNO);

	return -EINVAL;
	/* returns if execvp fails */
}

static int run_actions(void *args)
{
	int ret = 0;
	struct global_arg_s *gargs = (struct global_arg_s*)args;
	struct list_head *pos = NULL, *temp = NULL;
	struct excel_s *run_act = NULL;
	pid_t pid;
	
	list_for_each_safe(pos, temp, &(gargs->excel_tb)){
		run_act = list_entry(pos, struct excel_s, next);

		if(run_act){
			if(run_act->pid <= 0){ //没有对应的进程
				run_act->pid = vfork();
				if (run_act->pid < 0){
					log_error("can't fork");
					ret = run_act->pid;
					goto exit;
				}
				
				if (run_act->pid) {
					log_info("run_act->pid:%d",run_act->pid);
					ret = waite_client_running(&(gargs->child));
					if(ret){
						//杀掉不响应的进程
						log_error("waite_client_running(%d, %d)",ret,  process_mon((void*)&g_args, WNOHANG));
						kill_process(&(run_act->pid), run_act->command);
						goto exit;
					}
					//return pid; /* Parent  */
				}else{
					//child
					
					/* Create a new session and make ourself the process group leader */
					//setsid(); //当前进程异常时 所有子进程都要退出
					init_exec(run_act->command);
					//
					_exit(-1);
				}
			}
		}
	}

exit:
	return ret;
}

static int mark_terminated(pid_t pid, void *args)
{
	struct global_arg_s *gargs = (struct global_arg_s*)args;
	struct list_head *pos = NULL, *temp = NULL;
	struct excel_s *run_act = NULL;
	int ret = 0;
	
	list_for_each_safe(pos, temp, &(gargs->excel_tb)){
		run_act = list_entry(pos, struct excel_s, next);
		if(run_act){
			if ((run_act->pid == pid) || \
				(run_act->pid == waitpid(run_act->pid, NULL, WNOHANG))){
				log_error("process '%s' (pid %d) exited. "
					"Scheduling for restart.",
					run_act->command, run_act->pid);
				run_act->pid = -1;
				//gargs->child.excep_cnt--;
				//gargs->child.running_cnt--; //先在这减  目前没有设置遗嘱
				//return run_act;
				ret++;
			}
		}

	}
	
	return ret;
}

int kill_all(void *args)
{
	int ret = 0;
	struct global_arg_s *gargs = (struct global_arg_s*)args;
	struct list_head *pos = NULL, *temp = NULL;
	struct excel_s *run_act = NULL;

	list_for_each_safe(pos, temp, &(gargs->excel_tb)){
		run_act = list_entry(pos, struct excel_s, next);
		if(run_act){
			if (run_act->pid > 0) {
				ret |= kill(run_act->pid,SIGINT);
				log_info("kill process '%s' (pid %d).ret = %d ",
					run_act->command, run_act->pid, ret);
				run_act->pid = -1; //????
			}
		}
	}
	usleep(100000); //100ms 等待进程退出
	process_mon(args, WNOHANG); //回收资源
	return ret;
}


int process_mon(void *args, int options)
{
	int ret = 0;
	pid_t wpid;
	struct global_arg_s *gargs = (struct global_arg_s*)args;
	//struct excel_s *a;

	wpid = waitpid(-1, NULL, options); 
	if (wpid > 0){
		ret = mark_terminated(wpid, args);
	}

	return ret;
}

int transfer2dev(void *args)
{
	return 0;
}

int compare_ret_val(struct stat_s *st)
{
	int ret = 0;
	int i = 0, j = 0, k = 0;
	int reg_exp, mask_exp, rtv_exp;
	int reg, mask, rtv;
	
	for(i=0; i<st->item_exp; i++){
		reg_exp = st->ret_exp[i].reg;
		mask_exp = st->ret_exp[i].mask;
		rtv_exp = st->ret_exp[i].rt;
		for(j=0; j<st->item; j++){ //可以优化

			reg = st->ret[j].reg;
			
			if(reg == reg_exp){
				k++;
				//mask = st->ret[i].mask;
				rtv = st->ret[j].rt;
				if((mask_exp & rtv) != rtv_exp){ //返回值不对
					log_error("compare_ret_val err.reg:%d, mask: 0x%x, rtv: 0x%x, rtv_exp: 0x%x",\
								reg, mask_exp, rtv, rtv_exp);
					ret = -EINVAL;
					goto exit;
				}
			}
		}
		
	}

exit:
	return ret;
}

/*
*  1、发送配置文件， 获取执行状态(成功、失败)
*  2、按顺序发送statfile(对应设备配置文件的数据+ 期待返回值)
           对比是否出错
*/
int send_cmd2dev(void *args)
{
	int ret = 0, st;
	pid_t wpid;
	//time_t sec_beg;
	struct global_arg_s *gargs = (struct global_arg_s*)args;
	struct list_head *pos = NULL, *temp = NULL;
	struct list_head *pos_inner = NULL, *temp_inner = NULL;
	struct run_s *run_act = NULL;
	struct stat_s *stat_act = NULL;
	char *file_buf = NULL, *stat_buf;
	int stat_buf_len;
	char *st_buf = NULL;
	int ret_val, mask_val;
	int i;

	if(ST_M_DONE == gargs->st_m){
		goto exit;
	}
	list_for_each_safe(pos, temp, &(gargs->run_tb)){
		pthread_testcancel();
		run_act = list_entry(pos, struct run_s, next);
		if(run_act){
			pthread_testcancel(); //标C函数的线程取消点有问题， 这插入一个取消点
			if(0 == strcmp(run_act->dst, SHELL_STR)){
				//usleep(run_act->st_to);
				system(run_act->con_file);
				continue;
			}
			
			ret = read_config_file(run_act->con_file, &file_buf);
			if(ret < 0){
				log_warn("read conf file(%s).",run_act->con_file);
				//goto exit; //允许没有配置文件
			}else{
				log_info("conf_file:%s",run_act->con_file);
				
				pthread_testcancel(); //标C函数的线程取消点有问题， 这插入一个取消点
				// 发送配置文件并等待执行完成
				ret = dev_transfer(file_buf, ret, &stat_buf, &stat_buf_len, run_act->dst, -1);
				if(ret){
					log_error("dev_transfer err(%d)",ret);
					goto exit;
				}	
				if(stat_buf){
					ret = get_return_stat(stat_buf);//strstr(stat_buf, "done")?0:-EINVAL;//strcmp(stat_buf, "done");
					xfree(stat_buf);
				}
				
				if(ret < 0){
					log_error("run_act->dst respond err(%d)",ret);
					goto exit;
				}
			}
			
			//
			i = 0;
			list_for_each_safe(pos_inner, temp_inner, &(run_act->stat_tb)){
				stat_act = list_entry(pos_inner, struct stat_s, next);

				pthread_testcancel(); //标C函数的线程取消点有问题， 这插入一个取消点
				log_info("st_file:%s",run_act->st_file_tk[i]);
				ret = read_config_file(run_act->st_file_tk[i], &st_buf);
				if(ret < 0){
					log_error("get_st_file_ret_excp err(%d).",ret);
					goto exit;
				}
				pthread_testcancel(); //标C函数的线程取消点有问题， 这插入一个取消点
				//transfer
				ret = dev_transfer(st_buf, ret, &stat_buf, &stat_buf_len, stat_act->dst, -1);
				if(ret){
					log_error("dev_transfer err(%d)",ret);
					goto exit;
				}

				//
				ret = get_return_json_data(stat_buf, stat_act);
				if(ret < 0){
					log_error("get_return_json_data err(%d).",ret);
					goto exit;
				}

				ret = compare_ret_val(stat_act);
				if(ret < 0){
					log_error("compare_ret_val err(%d).",ret);
					goto exit;
				}
				i++;
					
				xfree(st_buf);
				//st_buf = NULL;
			}
		}
	}

//	gargs->st_m = ST_M_DONE;
	
exit:
	//if(file_buf){
		xfree(file_buf);
	//}

	//if(st_buf){
		xfree(st_buf);
	//}
	return ret;
}

#ifndef	USE_MQTT
int center2dev(const char* jsonStr, void* wptr, void *out)
{
	return 0;
}
#else
int client_born(char* buf, int buf_len, char *from, void* arg)
{
	int ret = 0;
	char *type;
	struct global_arg_s *args = (struct global_arg_s *)arg;
	
	//if(g_args.dbg){
		log_debug("%s send info:",from);
		print_msg(buf, buf_len);
	//}

	if(strstr(from, "/pub/born")){ //
		wakeup_exel(&(args->child));
	}
	
	return ret;
}

int client_exception(char* buf, int buf_len, char *from, void* arg)
{
	int ret = 0;
	struct err_jsonstr_s ret_str = {
		.str = err_jsonstr,
		.free = 0,
	};
	char *type;
	struct global_arg_s *args = (struct global_arg_s *)arg;
	
	if(args->dbg){
		log_debug("%s send info:",from);
		//print_msg(buf, buf_len);
	}

	//set_statemachine(args, ST_M_QUIT);
	set_statemachine(args, ST_M_EXIT);

	return ret;
}

#endif

static void* mon_thread(void *args)
{
	int ret;
	struct global_arg_s *gargs = (struct global_arg_s *)args;

	for(;;){
		do{
			ret = waite_statemachine(gargs, ST_M_FORK);
		}while(ret);
		
		ret = get_statemachine(gargs);
		if((ST_M_FORK|ST_M_DONE)& ret){
			ret = process_mon(args, 0); //等待
			if(ret){
				set_statemachine(gargs, ST_M_EXIT); //ST_M_QUIT
			}
		}
	}

	return 0;
}


//插入取消点的好处:某个进程可能意外退出，
//这时如果再发生req/respond 则可能造成长时间的阻塞
static void* send_thread(void *args)
{
	int ret;
	int i;
	int go_on;
	struct global_arg_s *gargs = (struct global_arg_s *)args;

	//for(;;){
		do{
			pthread_testcancel();
			ret = waite_statemachine(gargs, ST_M_FORK);
		}while(ret);
		
		go_on = 1;
		for(i=0; i< gargs->retry; i++){ //多次尝试
#ifdef 	get_statemachine
			get_statemachine(gargs);
			if(ST_M_FORK == ret){
#else
			if(ST_M_FORK == get_statemachine(gargs)){
#endif
				ret = send_cmd2dev(args);
				if(!ret){
					set_statemachine(gargs, ST_M_DONE);
					goto exit; //光荣完成使命
				}
			}else{ //有进程异常退出了  重新开始
				go_on = 0;
				break;
			}
		}
#ifdef 	get_statemachine	
		get_statemachine(gargs);
		if((ST_M_DONE != ret)&& go_on){
#else
		if((ST_M_DONE != get_statemachine(gargs))&& go_on){
#endif
			//异常通知
			set_statemachine(gargs, ST_M_EXIT);
		}
	//}

exit:
	return 0;
}


int main(int argc, char **argv)
{
	int ret = 0;
	struct timespec timeout;
	struct timeval now;
	int exit_cnt = 0;

	ret = global_args_init(&g_args);
	if(ret < 0){
		goto exit;
	}
		
	ret = get_cmd_line_arg(argc, argv, &g_args);
	if(-EPERM == ret){
		goto exit;
	}


	ret = mon_log_init(g_args.log_file, "mon_cat");
	if(ret){
		//日志失败，怎么处理????  
		//log_error("spidev_log_init err! log:%s",g_args.log_file);
		goto exit;
	}

#ifdef	USE_MQTT
	//snprintf(g_args.unique_id, ARRAY_SIZE(g_args.unique_id), "%d-%s",getpid(), g_args.prog_name);
	ret = init_broker(g_args.prog_name, NULL, -1, mqtt_disconnect_process, (void*)&g_args);

	if(ret){
		log_error("init_broker fail(%d)!",ret);
		goto exit;
	}

	ret = register_callback("all", client_exception, "event");
	if(ret){
		log_error("init_broker fail(%d)!",ret);
		goto exit1;
	}

	ret = register_callback("all", client_born, "born"); //等待接收 其它进程初始化完成消息
	if(ret){
		log_error("init_broker fail(%d)!",ret);
		goto exit2;
	}

#else
	if(!initThread(MON, current_logcat)){
		log_error("connect to center fail!");
		goto exit;
	}

	if(!registerCallbackFunc(center2dev,(void*)&g_args)){
		log_error("registerCallbackFunc fail!");
		goto exit;
	}
#endif
	ret = parse_inittab(g_args.conf_file, &g_args);
	if(ret < 0){
		log_error("parse_inittab fail!(%d)",ret);
		goto exit2;
	}

	if(g_args.dbg){
		print_all(&g_args);
	}

	prctl(PR_SET_PDEATHSIG,SIGHUP); //父进程退出 子进程也退出

	ret = pthread_create(&(g_args.mon_pid), NULL, mon_thread, (void*)&g_args);
	if(ret){
		log_error("pthread_create fail!(%d)", ret);
		goto exit2;
	}
	pthread_detach(g_args.mon_pid);
		
	for(;;){
		ret = pthread_create(&(g_args.sender_pid), NULL, send_thread, (void*)&g_args);
		if(ret){
			log_error("pthread_create fail!(%d)", ret);
			goto exit2;
		}

		process_mon((void*)&g_args, WNOHANG);
		ret = run_actions((void*)&g_args); 
		if(ret){
			log_error("run_actions fail!(%d)", ret);
			//unregister_callback(g_args.prog_name, NULL);
			goto exit3; 
		}

		set_statemachine(&g_args, ST_M_FORK);
	
		for(;;){
			//ret = process_mon((void*)&g_args, WNOHANG); //等待
			waite_statemachine_forever(&g_args, ST_M_QUIT|ST_M_EXIT );

			ret = get_statemachine(&g_args);
			if(ret == ST_M_QUIT){
				break;
			}else if(ret == ST_M_EXIT){
				goto exit3; 
			}
		}
		ret = pthread_cancel(g_args.sender_pid);

		if(ret && (ESRCH != ret)){
			//线程不可取消，即出现了异常
			log_error("pthread_cancel fail!(%d)", ret);
			pthread_detach(g_args.sender_pid);
			goto exit2;
		}
		pthread_join(g_args.sender_pid, NULL);
		set_statemachine(&g_args, ST_M_PARSE_RUN);
	}

exit3:
	log_error("exit");
	pthread_cancel(g_args.mon_pid);
	
	pthread_cancel(g_args.sender_pid);
	pthread_join(g_args.sender_pid, NULL); //会阻塞退出
	
exit2:		
#ifdef	USE_MQTT
	unregister_callback(g_args.prog_name, "born");
	unregister_callback(g_args.prog_name, "event");
#endif
	
exit1:
	close_broker();
	
exit:
	kill_all(&g_args);
	clean_inittab((void*)&g_args);
	global_args_clean(&g_args);

#ifndef	USE_MQTT
	freeThread();
#endif
	return ret;
}

