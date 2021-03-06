#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "core.h"
#include "utils.h"
#include "main.h"
#include "socket.h"
#include "tty.h"



/**
 * 
 */
void version(bool end)
{
	printf("%s v%.2f\n", PROGNAME, VERSION);
	if (end) {
		proxenet_xfree(cfg);
		exit(0);
	}
}


/**
 *
 */
void usage(int retcode)
{
	FILE* fd;
	fd = (retcode == 0) ? stdout : stderr;
	
	fprintf(fd,
		"\nSYNTAX :\n"
		"\t%s [OPTIONS+]\n"
		"\nOPTIONS:\n"
		"\t-t, --nb-threads=N\t\t\tNumber of threads (default: %d)\n"
		"\t-b, --lbind=bindaddr\t\t\tBind local address (default: %s)\n"
		"\t-p, --lport=N\t\t\t\tBind local port file (default: %s)\n"
		"\t-l, --logfile=/path/to/logfile\t\tLog actions in file\n"
		"\t-x, --plugins=/path/to/plugins/dir\tSpecify plugins directory (default: %s)\n"
		"\t-k, --key=/path/to/ssl.key\t\tSpecify SSL key to use (default: %s)\n"
		"\t-c, --cert=/path/to/ssl.crt\t\tSpecify SSL cert to use (default: %s)\n"
		"\t-v, --verbose\t\t\t\tIncrease verbosity (default: 0)\n"
		"\t-n, --no-color\t\t\t\tDisable colored output\n"
		"\t-4, \t\t\t\t\tIPv4 only (default: all)\n"
		"\t-6, \t\t\t\t\tIPv6 only (default: all)\n"
		"\t-h, --help\t\t\t\tShow help\n"
		"\t-V, --version\t\t\t\tShow version\n"
		"",
		
		PROGNAME,
		CFG_DEFAULT_NB_THREAD,
		CFG_DEFAULT_BIND_ADDR,
		CFG_DEFAULT_PORT,
		CFG_DEFAULT_PLUGINS_PATH,
		CFG_DEFAULT_SSL_KEYFILE,
		CFG_DEFAULT_SSL_CERTFILE);
	
	exit(retcode);
}


/**
 *
 */
void help(char* argv0)
{
	const char* plugin_name;
	int plugin_idx;
	
	version(false);
	printf("Written by %s\n"
	       "Released under: %s\n\n"
	       "Compiled with support for :\n",
	       AUTHOR, LICENSE);
	
	for(plugin_idx=0, plugin_name=supported_plugins_str[0];
	    plugin_name!=NULL;
	    plugin_idx++, plugin_name=supported_plugins_str[plugin_idx]) 
		printf("\t[+] 0x%.2x   %-10s (%s)\n",
		       plugin_idx,
		       plugin_name,
		       plugins_extensions_str[ plugin_idx ]);
	
	
	usage(EXIT_SUCCESS);
}


/**
 *
 */
bool parse_options (int argc, char** argv)
{
	int curopt, curopt_idx;
	char *path, *keyfile, *certfile;
	
	const struct option long_opts[] = {
		{ "help",       0, 0, 'h' },
		{ "verbose",    0, 0, 'v' },
		{ "nb-threads", 1, 0, 't' },
		{ "liface",     1, 0, 'b' },
		{ "lport",      1, 0, 'p' },
		{ "logfile",    1, 0, 'l' },
		{ "certfile",   1, 0, 'c' },
		{ "keyfile",    1, 0, 'k' },
		{ "plugins",    1, 0, 'x' },
		{ "no-color",   0, 0, 'n' },
		{ "version",    0, 0, 'V' },
		{ 0, 0, 0, 0 } 
	};
	
	cfg->iface		= CFG_DEFAULT_BIND_ADDR;
	cfg->port		= CFG_DEFAULT_PORT;
	cfg->logfile_fd		= CFG_DEFAULT_OUTPUT;
	cfg->nb_threads		= CFG_DEFAULT_NB_THREAD;
	cfg->use_color		= true;
	cfg->ip_version		= CFG_DEFAULT_IP_VERSION;

	path			= CFG_DEFAULT_PLUGINS_PATH;
	keyfile			= CFG_DEFAULT_SSL_KEYFILE;
	certfile		= CFG_DEFAULT_SSL_CERTFILE;
	
	while (1) {
		curopt = -1;
		curopt_idx = 0;
		curopt = getopt_long (argc,argv,"n46Vhvb:p:l:t:c:k:x:",long_opts, &curopt_idx);
		if (curopt == -1) break;
		
		switch (curopt) {
			case 'v': cfg->verbose++; break;
			case 'b': cfg->iface = optarg; break;
			case 'p': cfg->port = optarg; break;
			case 'l': cfg->logfile = optarg; break;
			case 't': cfg->nb_threads = (unsigned short)atoi(optarg); break;
			case 'c': certfile = optarg; break;
			case 'k': keyfile = optarg; break;	   
			case 'h': help(argv[0]); break;
			case 'V': version(true); break;
			case 'n': cfg->use_color = false; break;
			case '4': cfg->ip_version = AF_INET; break;
			case '6': cfg->ip_version = AF_INET6; break;
			case 'x': path = optarg; break;
			case '?':
			default:
				usage (EXIT_FAILURE);
		}
		curopt_idx = 0;
	}
	
	if(cfg->logfile && is_readable_file(cfg->logfile)) {
		cfg->logfile_fd = fopen(cfg->logfile, "a");
		if(!cfg->logfile_fd) {
			fprintf(stderr, "[-] Failed to open '%s': %s\n", cfg->logfile, strerror(errno));
			return false;
		}
	}
	
	if (cfg->nb_threads > MAX_THREADS) {
		fprintf(stderr, "Too many threads. Setting to default.\n");
		cfg->nb_threads = CFG_DEFAULT_NB_THREAD;
	}
	
	if (path && is_valid_path(path))
		cfg->plugins_path = realpath(path, NULL);
	else {
		xlog(LOG_CRITICAL, "%s\n", "Invalid plugins path provided");
		return false;
	}

	if ( is_readable_file(certfile) )
		cfg->certfile = realpath(certfile, NULL);
	else {
		xlog(LOG_CRITICAL, "Failed to read certificate '%s'\n", cfg->certfile);
		return false;
	}

	if ( is_readable_file(keyfile) )
		cfg->keyfile = realpath(keyfile, NULL);
	else {
		xlog(LOG_CRITICAL, "Failed to read private key '%s'\n", cfg->keyfile);
		return false;
	}
	
	return true;
}


/**
 *
 */
int proxenet_init_config(int argc, char** argv)
{
	cfg = &current_config;
	proxenet_xzero(cfg, sizeof(conf_t));
	
	if (parse_options(argc, argv) == false) {
		xlog(LOG_ERROR, "%s\n", "Failed to parse arguments");
		return -1;
	}

	return 0;
}


/**
 *
 */
void proxenet_free_config()
{
	proxenet_xfree(cfg->plugins_path);
	proxenet_xfree(cfg->certfile);
	proxenet_xfree(cfg->keyfile);
	
	if(cfg->logfile_fd)
		fclose(cfg->logfile_fd);
	
}


/**
 *
 */
int main (int argc, char **argv, char** envp)
{
	int retcode = -1;
	
	/* init semaphore for unified display */
	sem_init(&tty_semaphore, 0, 1);
	
	/* get configuration */
	retcode = proxenet_init_config(argc, argv);
	if (retcode)		
		goto end;
	
	tty_open();
	
	/* proxenet starts here  */
	
	retcode = proxenet_start(); 
	
	/* proxenet ends here */
end:
	tty_close();

	proxenet_free_config();
	
	return (retcode == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


