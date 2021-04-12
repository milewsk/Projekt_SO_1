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
    if (argc < 2)
    {
        syslog(LOG_ERR, "Zbyt mala liczba argumentow wejsciowych!");
        exit(EXIT_FAILURE);
    }
    
/*zainicjowanie zmiennych pod proces*/
    pid_t pid, sid;
    int i;
    
    /*Nowy proces*/
    pid = fork();
    if (pid == -1)
        return -1;
    else if (pid != 0)
        exit(EXIT_SUCCESS);

    umask(0);

   
    /*Nowa sesja i grupa procesów*/
    if ((sid = setsid()) == -1)
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

    /*
    Inicjalizacja podstawowych parametrów do pracy demona: 
    -ustawienie rekurencji na false
    - ustawienie domyślnego czasu uśpienia
    - domyślnego rozmiaru pliku (capacity)
    - inicjalizacja pustych wskaźników przechowujących w przyszłości ścieżki do folderów
    
    Struct stat - pozwala nam na wydobywanie informacji o danym folderze/pliku
    */
    
    int choice = 0;
    bool recursion = false;
    char *in, *out;
    int capacity = 1024;
    int sleep = 300; //300, gdyż w zadaniu podane jest 5 minut bazowe czyli 300 sekund.
    char *directory_path1 = NULL, *directory_path2 = NULL;
    struct stat s;

    /*Tutaj menu dla demona z funkcjami*/
    while ((choice = getopt(argc, argv, "i:o:c:rs")) != -1)
    {
        switch (choice)
        {
        /*Argument opcjonalny: Zmiana czasu uśpienia*/
        case 's': 
                //convert
            sleep = atoi(optarg);
            break;
        /*Argument i - scieżka do folderu wejściowego */
        case 'i':
            in = optarg;
            if (stat(in, &s) == 0)
            {
                //IFDIR - Sprawdzenie czy obiekt jest folderem
                
                // jeśli st_mode == IFdir, jeśli okej przypisz podany argument do zmiennej
                if (S_IFDIR(s.st_mode)) //lub s.st_mode & S_IFDIR
                {
                    directory_path1 = optarg;
                }
                //nie jest katalogiem
                else 
                {
                    syslog(LOG_ERR, "Podany argument 'i' nie jest folderem");
                    exit(EXIT_FAILURE);
                }
            }
            break;
        /*Argument o - scieżka do folderu wyjściowego, miejsce docelowe */
        case 'o':
            out = optarg;
                
            if (stat(out, &s) == 0)
            {
                //sprawdzenie czy obiekt jest katalogiem, jeśli okej przypisz podany argument do zmiennej
                if (S_IFDIR(s.st_mode)) 
                {
                    directory_path2 = optarg;
                }
                // S_IFDIR - false
                //nie jest katalogiem
                else 
                {
                    syslog(LOG_ERR, "Podany argument nie jest folderem");
                    exit(EXIT_FAILURE);
                }
            }
            break;
                
/*Argument opcjonalny: Zmiana trybu na rekurencyjny*/
        case 'r':
                //zmiana wartości na true
            recursion = true;
            break;
                
 /*Argument opcjonalny: Zmiana rozmiaru*/
        case 'c':
                //covert
            capacity = atoi(optarg);
            break;
        }
    }



    /*Dalsza część maina z daemonem*/
}
