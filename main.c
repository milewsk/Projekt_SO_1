#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h> //przetwarzanie sygnałów
#include <string.h>
#include <system.h> //
#include <unistd.h> //udostępnia makra i funkcje niezbędne do tworzenia programów, które muszą korzystać z pewnych usług systemu operacyjnego. getopt będzie z tego brany
#include <sys/types.h> //zawiera definicje dla pid_t itd.
#include <sys/stat.h> //The <sys/stat.h> header defines the structure of the data returned by the functions fstat(), lstat(), and stat().
#include <fcntl.h> //file control options
#include <linux/fs.h> //Ten plik zawiera definicje pewnych ważnych struktur, tablic plików itp.
#include <dirent.h> //plik nagłówkowy w bibliotece POSIX języka C. Udostępnia funkcje, makra, i struktury, które umożliwiają łatwe trawersowanie katalogów.
#include <utime.h> //Pozwala programistom zmieniać i ustawiać datę dostępu oraz datę modyfikacji plików.
#include <sys/mman.h> //Memory management - będzie potrzebne do mapowania plików void  *mmap(void *, size_t, int, int, int, off_t);

/*Tutaj funkcje
Potrzebne nam funkcje:
Pobieranie czasu/rozmiaru/poziomu uprawnień
Zmiana katalogów
Usuwanie katalogów
Sprawdzanie folderów. Pozycje które nie są zwykłymi plikami są ignorowane (np. katalogi i dowiązania symboliczne).*/


//Źródło https://man7.org/linux/man-pages/man2/lstat.2.html
off_t FileSize(const char* in)
{
    struct stat fsize;
    if (stat(in, &fsize) == 0)
    {
        return fsize.st_size;
    }
    else
        return -1;
}

mode_t FileTypeMode (const char* in)
{
    struct stat filetypemode;
    if (stat(in, &filetypemode) != -1)
    {
        return filetypemode.st_mode;
    }
    else
        return -1;
}

time_t FileModificationData(const char* in)
{
    struct stat time;
    if (stat(in, &time) == -1)
    {
        exit(EXIT_FAILURE);
    }
    return time.st_mtime;
}



int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        syslog(LOG_ERR, "Zbyt mala liczba argumentow wejsciowych!");
        exit(EXIT_FAILURE);
    }
    
    pid_t pid, sid;
    int i;
    
    /*Nowy proces*/
    pid = fork();
    if (pid == -1)
        return -1;
    else if (pid != 0)
        exit(EXIT_SUCCESS);

    umask(0);

    sid = setsid();
    /*Nowa sesja i grupa procesów*/
    if (sid() == -1)
        return -1;

    /*Ustaw katalog roboczy na główny*/
    if (chdir("/") == -1)
        return -1;

    /*Zamknij wszystkie pliki otwarte*/

    for (i = 0; i < NR_OPEN; i++)//NR_OPEN to trochę overkill, ale spoko.
        close(i);

    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    int choice = 0;
    bool recursion = false;
    char *in, *out;
    int capacity = 1024;
    int sleep = 300; //300, gdyż w zadaniu podane jest 5 minut bazowe czyli 300 sekund.
    char *directory_path1=NULL, *directory_path2=NULL;
    struct stat s;

    while ((choice = getopt(argc, argv, "i:o:c:rs")) != -1)
    {
        switch (choice)
        {
        case 's': //argument z nowa wartoscia spania demona
            sleep = atoi(optarg);
            break;

        case 'i':
            in = optarg;
            if (stat(in, &s) == 0)
            {
                if (s.st_mode & S_IFDIR) //Ścieżka jest katalogiem
                {
                    directory_path1 = optarg;
                }
                else //Ścieżka nie jest katalogiem
                {
                    printf("-i: Podany argument nie jest folderem");
                    syslog(LOG_ERR, "Podany argument nie jest folderem");
                    exit(EXIT_FAILURE);
                }
            }
            break;

        case 'o':
            out = optarg;
            if (stat(out, &s) == 0)
            {
                if (s.st_mode & S_IFDIR) //Ścieżka jest katalogiem
                {
                    directory_path2 = optarg;
                }
                else //Ścieżka nie jest katalogiem
                {
                    printf("-o: Podany argument nie jest folderem");
                    syslog(LOG_ERR, "Podany argument nie jest folderem");
                    exit(EXIT_FAILURE);
                }
            }
            break;

        case 'r':
            recursion = true;
            break;

        case 'c':
            capacity = atoi(optarg);
            break;
        }
    }

}
