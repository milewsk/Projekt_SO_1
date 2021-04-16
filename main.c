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

void ChangeMod(char* in, char* out)
{
    struct utimbuf time;
    time.actime = 0; ///* The new access time. access time - The last time the file was accessed/opened by some command or application*/
    time.modtime = FileModificationTime(in);/* The new modification time (modify time) - The last time the file's content was modified. */
    if (utime(out, &time) != 0)
    {
        syslog(LOG_ERR, "Blad zmiany daty!");
        exit(EXIT_FAILURE);
    }
    mode_t OldFile = FileTypeMode(in);
    if (chmod(out, OldFile) != 0)
    {
        syslog(LOG_ERR, "Blad ustawienia uprawnien!");
        exit(EXIT_FAILURE);
    }
}
/*Return Value
0
utime() was successful. The file access and modification times are changed.
-1
utime() was not successful. The file times are not changed. The errno global variable is set to indicate the error.*/



void copy_(char *in, char *out)
{
    char bufor[16];
    int file_in, file_out;
    int read_in, read_out;
    //czytanie z pliku wejściowego READONLY
    file_in = open(in, O_RDONLY)
    // pisanie do pliku wyjściowego
        //zgoda na tworzenie plików
    file_out = open(out, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if(file_in == -1 || fileout == -1)
    {
        syslog(LOG_ERR, "Błąd pliku podczas kopiowania/otwierania folderu");
    }
    
    //kopiowanie
    //czytanie z read_in i zapisywanie write() do read_out
    while((read_in = read(file_in, bufor, sizeof(bufor)))>0)
    {
    read_out = wirte(file_out, bufor, (size_t) read_in);
        //jeśli dwa pliki się nie zgadzają
        if(read_out != read_in)
        {
            //idk czy okej?
            syslog(LOG_ERR, "Błąd w kopiowaniu pliku");
            perror("Błąd w kopiowaniu pliku");
            exit(EXIT_FAILURE);
        }
    }
    //zamknięcie połączeń
    close(file_in);
    close(file_out);
    //zmiana parametrów skopiowanego pliku
    ChangeMod(in, out);
    
    //Odpowiedź jeśli wyszystko poszło zgodnie w planem
    syslog(LOG_INFO, "Kopiowanie udane! Skopiowano %s", in);
}

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

    for (i = 0; i < NR_OPEN; i++)//NR_OPEN to trochę overkill, ale spoko.
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
    int sleep_time = 300; //300, gdyż w zadaniu podane jest 5 minut bazowe czyli 300 sekund.
    char *directory_path1 = NULL, *directory_path2 = NULL;
    struct stat s;

    while ((choice = getopt(argc, argv, "i:o:c:rs")) != -1)
    {
        switch (choice)
        {
        /*Argument opcjonalny: Zmiana czasu uśpienia*/
        case 's': 
                //convert
            sleep_time = atoi(optarg);
            break;
        /*Argument i - scieżka do folderu wejściowego */
        case 'i':
            in = optarg;
            if (stat(in, &s) == 0)
            {
                //IFDIR - Sprawdzenie czy obiekt jest folderem
                
                // jeśli st_mode == IFdir, jeśli okej przypisz podany argument do zmiennej
                if (S_IFDIR(s.st_mode) != 0) //lub s.st_mode & S_IFDIR
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
                if (S_IFDIR(s.st_mode) != 0) 
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
    
    
    //ciągła pętla while podczas działania deamona
    while(1)
    {
        //potrzebne do funkcji: scieżki, rozmiar rekurencja true/false
        
        //usuwanie
        // program porównuje folder 2 z folderem 1
        // jeśli napotka niepotrzebne, niezgodne pliki usunie je
        
        
        //porównanie
        // program porównuje folder 1 i drugi
        // dodaje brakujące pliki i foldery
        
        
        
        //odpoczynek na czas 'sleep_time'
        syslog(LOG_INFO,"Synchronizacja wykonana, pora odpocząć...")
        
        //przebudzienie
            // sleep()
        if(sleep(sleep_time) == 0)
        {
            syslog(LOG_INFO,"Przebudzenie programu");
        }
        
    }
    
    

}
