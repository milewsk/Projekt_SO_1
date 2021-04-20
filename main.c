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
    //O_WRONLY - only writing
    //O_CREAT | O_WRONLY | O_TRUNC
    // CREAT = twórz jeśli nie istnieje taki plik
    //O_TRUNC = the file is successfully opened O_RDWR or O_WRONLY,
    //its length shall be truncated to 0, and the mode and owner shall be unchanged.
    // 0644 - owner = może wszystko grupa = czytać, inni = czytać 
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

void Map(char* in, char* out)
{
    /* mmap() creates a new mapping in the virtual address space of the
       calling process.  The starting address for the new mapping is
       specified in addr.  The length argument specifies the length of
       the mapping (which must be greater than 0).*/
    int size = FileSize(in);
    int FileInput = open(in, O_RDONLY); //O_RDONLY Open for reading only.
    int FileOutput = open(out, O_CREAT | O_WRONLY | O_TRUNC, 0644); /*If the file exists, this flag has no effect except as noted under O_EXCL below. Otherwise, the file shall be created O_WRONLY
    Open for writing only. If the file exists and is a regular file, and the file is successfully opened O_RDWR or O_WRONLY, its length shall be truncated to 0, and the mode and owner shall be unchanged.*/

    if (FileInput == -1 || FileOutput == -1)
    {
        syslog(LOG_ERR, "Blad otwarcia ktoregos z pliku!");
        exit(EXIT_FAILURE);
    }
    /*mmap() creates a new mapping in the virtual address space of the calling process.*/
    char* mapowanie = (char*)mmap(0, size, PROT_READ, MAP_SHARED | MAP_FILE, FileInput, 0);
    /*PROT_READ
              Pages may be read.

      MAP_SHARED
              Share this mapping.  Updates to the mapping are visible to
              other processes mapping the same region

      MAP_FILE
              Compatibility flag.*/

    write(FileOutput, mapowanie, size);
    //Mapowanie w miejsce buffora
    close(FileInput);
    close(FileOutput);
    munmap(mapowanie, size); //usuwanie mapy z paamieci;
    ChangeMod(in, out); //Zmiana daty modyfikacji, tak aby przy kolejnym obudzeniu nie trzeba było wykonać kopii
    syslog(LOG_INFO, "Z uzyciem mapowania skopiowano plik %s do miejsca %s", in, out);
}

//fukcja działająca w dwie strony w zależności
// jakie ścieżki podstawimy w odpowiednie miejsca
// kiedy chcemy podmienić folder:
// z folderu 2 -> folder 1 (uzupełnianie braków) (,folder2,folder1)
// z folderu 1 -> folder 2 ( ,folder1, folder2)

char *folder_replace(char* path, char* path_folder1, char* path_folder2)
{
    //
    char* form_path = path + strlen(path_folder1);
    //lokacja pamięci na nową scieżkę łącząc długosci podanych ścieżek
    char* new_path = malloc(strlen(path_folder2)+strlen(form_path)+1);\
//  ścieżka folder 2 na początek         
    strcpy(new_path, path_folder2);
//  doklejamy resztę
    strcat(new_path, form_path);
    return new_path;
}

// dodawanie brakujących elementów do ścieżki
char *add_to_path(char *path, char *added)
{
//     alokacja pamięci dla 'added' = 2 aby pomieścić \0
    char *new_path = malloc(strlen(path)+strlen(added)+2);
//     na początek scieżka początkowa
    strcpy(new_path,path);
//  dodajemy '/'
    strcat(new_path,"/");
//     dodajemy nasz 'added'
    strcat(new_path,added);
//     na koniec dodajemy zakończenie
    strcat(new_path,"\0")
}

// https://man7.org/linux/man-pages/man3/readdir.3.html
// na podstawie tego
void delete_(char* name_path_folder2, char*  path_folder1, char* path_folder2, bool if_R)
{
//  do sprawdzania typu i chodzenia głebiej w folderze
    struct dirent* file;
//     deklaracja dwóch nowych scieżek
    DIR* path, *temp;
// podstawienie w miejsce path scieżki forlderu2 za pomocą otworzenia
    path = opendir(name_path_folder2);
//     sprawdzenie czy scieżka jest faktycznie scieżką do folderu
//     jeśli jest usuń folder
    while(file = readdir(path)) 
    {
        if((file->d_type)==DT_DIR)
        {
            if(if_R)
            {
                if(!(strcmp(file->d_name,".")==0 || strcmp(file->d_name,"..")==0)) // jeśli jest dalsza część
                {
                    // utworzenie dalszej ścieżki
                    char* new_path = add_to_path(name_path_folder2,file->d_name);
                    // wywołanie rekurencujne usuwania
                    delete_(new_path,path_folder1,path_folder2,if_R);
                    
                    // jeśli nie możemy odszukać dalszego folderu zgodnego z folderem 1 
                    if(!(temp = opendir(folder_replace(new_path, path_folder2, path_folder1))))
                    {
                        if(remove(new_path) == 0)
                        {
                            syslog(LOG_ONFO, "USUNIĘTO katalog %s", new_path);
                        }
                        else
                        {
                            syslog(LOG_ERROR, "Błądy przy usuwaniu katalogu %s", new_path);
                        }
                    }
                    else // jeśli wszystko się zgadza
                    {
                        closedir(temp);
                    }
                }
            }
    
        }
        else // jeśli nie mamy włączonej rekurencji usuń nierekurencujnie                   tu też czy to w tym miejscu
        {
            // dalsza ścieżka do katalogu
            char* new_path = add_to_path(name_path_folder2, file->name)                                 // to dobrze na pewno???
           
                //spawdzamy czy damy radę zrobić replace == w obu katalogach jest folder
                // jeśli nie to usuwamy zbędy katalog
            if(access(folder_replace(new_path, path_folder2, path_folder1),F_OK)== -1)
            {
                  if(remove(new_path) == 0)
                        {
                            syslog(LOG_ONFO, "USUNIĘTO katalog %s", new_path);
                        }
                        else
                        {
                            syslog(LOG_ERROR, "Błądy przy usuwaniu katalogu %s", new_path);
                        }
                
            }
        }
    }
    // jeśli wszystko się zgadza - koniec usuwania
closedir(path);
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
