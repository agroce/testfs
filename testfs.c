#define _GNU_SOURCE
#include <stdbool.h>
#include <getopt.h>
#include "testfs.h"
#include "super.h"
#include "inode.h"
#include "dir.h"
#include "tx.h"

static int cmd_help(struct super_block *, struct context *c);
static int cmd_quit(struct super_block *, struct context *c);
static bool can_quit = false;

#define PROMPT printf("%s", "% ")

static struct {
	const char *name;
	int (*func)(struct super_block *sb, struct context *c);
} cmdtable[] = {
        /* menus */
        { "?",          cmd_help },
        { "cd",         cmd_cd },
        { "pwd",        cmd_pwd },
        { "ls",         cmd_ls },
        { "lsr",        cmd_lsr },
        { "touch",      cmd_create },
        { "stat",       cmd_stat },
        { "rm",         cmd_rm },
        { "mkdir",      cmd_mkdir },
        { "cat",        cmd_cat },
        { "write",      cmd_write },
        { "checkfs",    cmd_checkfs },
        { "quit",    	cmd_quit },
        { NULL,         NULL}
};

static int
cmd_help(struct super_block *sb, struct context *c)
{
        int i = 0;
        
        printf("Commands:\n");
        for (; cmdtable[i].name; i++) {
                printf("%s\n", cmdtable[i].name);
        }
        return 0;
}

static int
cmd_quit(struct super_block *sb, struct context *c)
{
        printf("Bye!\n");
        can_quit = true;
        return 0;
}
	
void
command(struct super_block *sb, struct context *c)
{
    int i;
    if (!c->cmd[0])
        return;

    for (i=0; cmdtable[i].name; i++) {
        if (strcmp(c->cmd[0], cmdtable[i].name) == 0) {
            assert(cmdtable[i].func);
                errno = cmdtable[i].func(sb, c);
                if (errno < 0) {
                    errno = -errno;
                    WARN(c->cmd[0]);
                }
                return;
            }
    }
    printf("%s: command not found: type ?\n", c->cmd[0]);
}

static void 
usage(const char * progname)
{
    fprintf(stderr, "Usage: %s [-ch][--help] rawfile\n", progname);
    exit(1);
}

struct args
{
    const char * disk;  // name of disk
    int corrupt;        // to corrupt or not
};

static struct args *
parse_arguments(int argc, char * const argv[])
{
    static struct args args = { 0 };
    static struct option long_options[] =
    {
        {"corrupt", no_argument,       0, 'c'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0},
    };
    int running = 1;
    
    while (running)
    {
        int option_index = 0;
        int c = getopt_long (argc, argv, "ch", long_options, &option_index);
        switch (c)
        {
        case -1:
            running = 0;
            break;
        case 0:
            break;
        case 'c':
            args.corrupt = 1;
            break;
        case 'h':
            usage(argv[0]);
            break;
        case '?':
            usage(argv[0]);
            break;
        default:
            abort();
        }
    }
    
    if ( argc - optind != 1 )
        usage(argv[0]);
        
    args.disk = argv[optind];
    return &args;
}

int
main(int argc, char * const argv[])
{
        struct super_block *sb;
        char *line = NULL;
        char *token;
        size_t line_size = 0;
        ssize_t nr;
        int ret;
        struct context c;
        struct args * args = parse_arguments(argc, argv);
        
        ret = testfs_init_super_block(args->disk,args-> corrupt, &sb);
        if (ret) {
            EXIT("testfs_init_super_block");
        }
        c.cur_dir = testfs_get_inode(sb, 0); /* root dir */
        for (;	
            PROMPT, 
            (nr = getline(&line, &line_size, stdin)) != EOF; ) {
            token = line;
            c.nargs = 0;
            bzero(c.cmd, sizeof(char *) * MAX_ARGS);
            while ((token = strtok(token, " \t\n")) != NULL) {
                    if (c.nargs < MAX_ARGS)
                            c.cmd[c.nargs++] = token;
                    token = NULL;
            }
            command(sb, &c);
                if ( can_quit ) break;
        }
        testfs_put_inode(c.cur_dir);
        testfs_close_super_block(sb);
        return 0;
}
