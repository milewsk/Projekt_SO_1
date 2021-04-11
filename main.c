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

/*Tutaj przydałoby się wypisać prototypy*/




/*Tutaj funkcje
Potrzebne nam funkcje:
Pobieranie czasu/rozmiaru/poziomu uprawnień
Zmiana katalogów
Usuwanie katalogów
Sprawdzanie folderów. Pozycje które nie są zwykłymi plikami są ignorowane (np. katalogi i dowiązania symboliczne).*/

int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        printf("Zbyt mala liczba argumentow wejsciowych!");
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
    if (setsid() == -1)
        return -1;

    /*Ustaw katalog roboczy na główny*/
    if (chdir("/") == -1)
        return -1;

    /*Zamknij wszystkie pliki otwarte*/

    for (i = 0; i < NR_OPEN; i++)
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

    /*Tutaj menu dla demona z funkcjami*/
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
                if (s.st_mode & S_IFDIR) //sciezka jest katalogiem
                {
                    directory_path1 = optarg;
                }
                else //sciezka nie jest katalogiem, wywal blad
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
                if (s.st_mode & S_IFDIR) //sciezka jest katalogiem
                {
                    directory_path2 = optarg;
                }
                else //sciezka nie jest katalogiem, wywal blad
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



    /*Dalsza część maina z daemonem*/
}
