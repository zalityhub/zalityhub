/*
 *  SocketStat v1.0 - by Richard Steenbergen <humble@lightning.net> and
 *  Drago <Drago@Drago.com>. Inspired by dreams, coded by nightmares.
 *
 *  Advantages:
 *    - Nifty way to find which processes are using what sockets
 *    - Can be used to detect users who clone on irc, connect where they
 *      shouldn't (bots on non-bot servers), are running hidden servers, etc.
 *  Disadvantages:
 *    - Must be suid root in order to display sockets other then your own
 *    - Kinda duplicates fuser and lsof but hey we had fun writing it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#define error(x)        { fprintf(stderr, "sockstat: %s\n", x); }
#define fatal(x)        { fprintf(stderr, "sockstat: %s\n", x); exit(2); }

#define SEARCH_ALL      0       /* Display info on all sockets */
#define SEARCH_GID      1       /* Search by a specific group/gid */
#define SEARCH_PID      2       /* Search by a specific process/pid */
#define SEARCH_PNAME    3       /* Search by a specific process name */
#define SEARCH_UID      4       /* Search by a specific user/uid */

#define PROTOCOL_TCP    3
#define PROTOCOL_UDP    2
#define PROTOCOL_RAW    1

typedef struct {
   ino_t inode;
   struct in_addr local_addr, remote_addr;
   u_int local_port, remote_port;
   u_char status, protocol;
   uid_t uid;
} ProcNet;

char *states[] = {
   "ESTBLSH",   "SYNSENT",   "SYNRECV",   "FWAIT1",   "FWAIT2",   "TMEWAIT",
   "CLOSED",    "CLSWAIT",   "LASTACK",   "LISTEN",   "CLOSING",  "UNKNOWN"
};

uid_t o_uid;
gid_t o_gid;
pid_t o_pid;
char buf[128], o_pname[8];
DIR *proc, *fd;
FILE *tcp, *udp, *raw;
ProcNet *NetData;
u_char o_search = SEARCH_ALL;
u_int total = 0, stattcp = 0, statudp = 0, statraw = 0;

void usage(char *progname)
{
   fprintf(stderr, "usage: %s [-u uid|user] [-g gid|group] [-p pid|process]\n",
           progname);
   exit(1);
}

int compare(const void *a, const void *b)
{
   ProcNet *a_rec, *b_rec;

   a_rec = (ProcNet *) a;
   b_rec = (ProcNet *) b;

   if (a_rec->inode == b_rec->inode)
      return 0;
   else
      return (a_rec->inode > b_rec->inode)?(1):(-1);
}

int read_tcp_udp_raw(char *buf, int bufsize)
{
   static char fc = PROTOCOL_TCP;
   FILE *fileptr;

change:
   switch(fc) {
      case PROTOCOL_TCP:
         fileptr = tcp;
         break;
      case PROTOCOL_UDP:
         fileptr = udp;
         break;
      case PROTOCOL_RAW:
         fileptr = raw;
         break;
      case 0:
         return 0;
      default:
         fatal("Program go down the hole.");
   }

   if (fgets(buf, bufsize, fileptr) != NULL)
      return fc;

   --fc;
   goto change;
}

char *get_program_name(char *pid) {
   char *ret;
   FILE *fp;

   if ((ret = malloc(8)) == NULL)
      fatal("Unable to allocate memory.");

   snprintf(buf, sizeof(buf), "/proc/%s/status", pid);

   if ((fp = fopen(buf, "r")) == NULL)
      goto error;

   if (fgets(buf, sizeof(buf), fp) == NULL)
      goto error;

   if (sscanf(buf, "Name: %s\n", ret) != 1)
      goto error;

   fclose(fp);
   return ret;

error:
   fclose(fp);
   return "unknown";
}

void display_record(ProcNet *Record, pid_t pid, char *pname)
{
   struct passwd *pwd;

   if (Record->protocol == PROTOCOL_TCP) printf("TCP ");
      else if (Record->protocol == PROTOCOL_UDP) printf("UDP ");
         else printf("RAW ");
   pwd = getpwuid(Record->uid);
   pname[7] = '\0';
   pwd->pw_name[8] = '\0';

   printf("%-8s ", pwd->pw_name);
   snprintf(buf, sizeof(buf), "%s[%u]", pname, pid);
   printf("%s%*s", buf, 15 - strlen(buf), "");
   snprintf(buf, sizeof(buf), "%s:%u ", inet_ntoa(Record->local_addr),
            Record->local_port);
   printf("%s %*s", buf, 21 - strlen(buf), "");
   snprintf(buf, sizeof(buf), "%s:%u", inet_ntoa(Record->remote_addr),
            Record->remote_port);
   printf("%s %*s", buf, 21 - strlen(buf), "");
   printf("%s\n", states[Record->status - 1]);

   switch(Record->protocol) {
      case PROTOCOL_TCP:
         ++stattcp;
         break;
      case PROTOCOL_UDP:
         ++statudp;
         break;
      case PROTOCOL_RAW:
         ++statraw;
         break;
   }
}

void read_proc_net(void)
{
   unsigned int i = 0, size = 256;
   char protocol;

   if ((NetData = calloc(sizeof(ProcNet), size)) == NULL)
      fatal("Unable to allocate memory");

   while ((protocol = read_tcp_udp_raw(buf, sizeof(buf))) != 0) {
      if (i == size) {
         size *= 2;
         if ((NetData = realloc(NetData, (sizeof(ProcNet) * size))) == NULL)
            fatal("Unable to allocate memory");
      }

      if (sscanf(buf, "%*u: %lX:%x %lX:%x %hx %*X:%*X %*x:%*X %*x %u %*u %u",
                 (u_long *)&NetData[i].local_addr, &NetData[i].local_port,
                 (u_long *)&NetData[i].remote_addr, &NetData[i].remote_port,
                 (u_short *)&NetData[i].status, (u_int *)&NetData[i].uid,
                 (u_int *)&NetData[i].inode) != 7)
         continue;

      NetData[i++].protocol = protocol;
   }

   total = i;
   qsort(NetData, total, sizeof(ProcNet), compare);
}

int main(int argc, char *argv[])
{
   struct passwd *pwd;
   struct group  *grp;
   struct dirent *procent, *fdent;
   int ch, i;

   printf("SocketStat v1.0 - A Socket Information Program\n\n");

   while ((ch = getopt(argc, argv, "g:u:p:h")) != EOF)
      switch(ch) {
         case 'g':                              /* Search a single group */
            o_search = SEARCH_GID;
            if ((grp = getgrnam(optarg)) == NULL)
               o_gid = atoi(optarg);
            else
               o_gid = grp->gr_gid;
            o_uid = atoi(optarg);
            break;
         case 'p':                              /* Display a single pid */
            o_search = SEARCH_PID;
            for(i=0;i<strlen(optarg);++i)
               if (!isdigit(optarg[i])) {
                  o_search = SEARCH_PNAME;
                  strncpy(o_pname, optarg, sizeof(o_pname));
               }
            if (o_search == SEARCH_PID)
               o_pid = (int)strtol(optarg, (char **)NULL, 10);
            break;
         case 'u':                              /* Search a single user */
            o_search = SEARCH_UID;
            if ((pwd = getpwnam(optarg)) == NULL)
               o_uid = atoi(optarg);
            else
               o_uid = pwd->pw_uid;
            break;
         case 'h':
         default:
            usage(argv[0]);
      }

   if ((tcp = fopen("/proc/net/tcp", "r")) == NULL)
      fatal("Cannot open /proc/net/tcp");

   if ((udp = fopen("/proc/net/udp", "r")) == NULL)
      fatal("Cannot open /proc/net/udp");

   if ((raw = fopen("/proc/net/raw", "r")) == NULL)
      fatal("Cannot open /proc/net/raw");

   if ((proc = opendir("/proc")) == NULL)
      fatal("Cannot open /proc/net/tcp");

   read_proc_net();

   fclose(tcp); fclose(udp); fclose(raw);

   printf("Pro User     Process        Local Address         Remote Address        State\n");

   while ((procent = readdir(proc)) != NULL) {
      if (!isdigit(*(procent->d_name)))
         continue;

      snprintf(buf, sizeof(buf), "/proc/%s/fd/", procent->d_name);

      if ((fd = opendir(buf)) == NULL)
         continue;

      while((fdent = readdir(fd)) != NULL) {
         struct passwd *pwd;
         struct group  *grp;
         struct stat st;
         ProcNet *ptr;
         char *pn;

         snprintf(buf, sizeof(buf), "/proc/%s/fd/%s", procent->d_name, fdent->d_name);
         if (stat(buf, &st) < 0)
            continue;
         if (!S_ISSOCK(st.st_mode))
            continue;

         if ((ptr = bsearch(&st.st_ino, NetData, total, sizeof(ProcNet), compare)) != NULL) {
            pn = get_program_name(procent->d_name);

            switch(o_search) {
               case SEARCH_PID:
                  if (o_pid == atoi(procent->d_name))
                     goto display;
                  break;
               case SEARCH_PNAME:
                  if (!strncasecmp(pn, o_pname, 8))
                     goto display;
                  break;
               case SEARCH_GID:
                  grp = getgrgid(o_gid);
                  while((pwd = getpwnam(*((grp->gr_mem)++))) != NULL)
                     if (pwd->pw_uid == ptr->uid)
                        goto display;
                  break;
               case SEARCH_UID:
                  if (o_uid == ptr->uid)
                     goto display;
                  break;
               case SEARCH_ALL:
                  goto display;
               default:
                  fatal("Program go down the hole.");
            }

            continue;
display:
            display_record(ptr, atoi(procent->d_name), pn);
         }
      }
   }

   if (stattcp + statudp + statraw)
      printf("Total: %d  (TCP: %d   UDP: %d   RAW: %d)\n", stattcp + statudp +
             statraw, stattcp, statudp, statraw);
   else
      printf("None.\n");

   exit(0);
}
