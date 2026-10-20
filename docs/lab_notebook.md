# Appunti Benchmark posix thread: Installazione e Configurazione

## Introduzione e Motivazioni della Ricerca

Questi appunti documentano la preparazione dell'ambiente di test e le procedure di benchmarking per il progetto di tesi. L'obiettivo principale della tesi è sviluppare una libreria di thread in user-space in C++ e analizzarne le prestazioni in scenari di concorrenza

Le configurazioni tecniche riportate in questo documento sono guidate dalle seguenti motivazioni:

*   **Baseline Motivazionale (kernel vs User):** I thread POSIX (pthreads), operando su Linux come kernel-level threads interamente gestiti dal sistema operativo, fungono da baseline iniziale. Il confronto con essi serve unicamente a misurare il costo e l'inefficienza delle costanti transizioni (context-switch) tra user-mode e kernel-mode, giustificando così l'esistenza della tesi e la necessità di un approccio interamente user-space.
*   **Confronto Architetturale in User-Space:** L'analisi principale delle prestazioni avverrà confrontando il modulo sviluppato con altre librerie che operano in user-space. Permettendo di valutare l'efficienza di vari algoritmi di scheduling.
*   **Isolamento su Singolo Core (Modello 1:N):** I test sono progettati per l'esecuzione su un singolo core. Adottando il modello 1:N (molti user threads mappati su un unico kernel thread), si isola e si valuta l'efficienza pura dello scheduling in user-space (che includerà algoritmi preemptive come Round Robin e MLFQ).

---

## 1. Installazione di Google Benchmark dai Sorgenti

**Motivazione:** L'installazione tramite il pacchetto precompilato (`sudo apt-get install libbenchmark-dev`) genera il warning `***WARNING*** Library was built as DEBUG`, rendendo i test inaffidabili. La compilazione dai sorgenti garantisce una build ottimizzata.

**Prerequisiti:**
```bash
sudo apt update
sudo apt install cmake
```

**Procedura di compilazione e installazione:**
1. **Clonare la repository:**
   ```bash
   mkdir -p ~/src
   cd ~/src
   git clone https://github.com/google/benchmark.git
   cd benchmark
   ```
2. **Creare la directory di build:**
   ```bash
   cmake -E make_directory "build"
   ```
   *(Utilizza CMake per creare la cartella al posto del classico comando Linux `mkdir`).*
   
3. **Configurare CMake:**
   ```bash
   cmake -E chdir "build" cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_TESTING=OFF ../
   ```
   * `-E chdir "build"` entra nella cartella appena creata.
   * `-DCMAKE_BUILD_TYPE=Release` rimuove il codice di debug garantendo una compilazione ottimizzata.
   * `-DBENCHMARK_ENABLE_TESTING=OFF` disabilita la compilazione dei file di test interni della libreria.
   
4. **Compilare la libreria:**
   ```bash
   cmake --build "build" --config Release
   ```
   *(Traduce i file sorgente C++ nei binari effettivi).*
   
5. **Installare nel sistema:**
   ```bash
   sudo cmake --build "build" --config Release --target install
   ```
   *(Trasferisce la libreria compilata nelle directory di sistema protette, tipicamente `/usr/local/lib` per i binari e `/usr/local/include` per gli header).*

> **Nota:** Una volta concluso, il compilatore `g++` troverà in automatico Google Benchmark linkandolo con il flag `-lbenchmark`.

---

## 2. Ambiente Isolato e Gestione Frequenze CPU

**Problema:** Il sistema operativo scala dinamicamente le frequenze della CPU per il risparmio energetico, causando instabilità e rumore nelle misurazioni.
**Prerequisiti:** `sudo apt install linux-tools-common linux-tools-generic`

### Comandi Manuali
- **Blocco al 100%:** `sudo cpupower frequency-set --governor performance` (Disabilita il risparmio energetico).
- **Ripristino:** `sudo cpupower frequency-set --governor powersave` (Fondamentale da eseguire a fine test per evitare consumi e surriscaldamenti inutili).

### Automazione tramite Wrapper (`pmake`)
Per isolare in automatico il processo di build ed esecuzione, ho scritto lo script nel file `~/.bashrc`:

```bash
pmake() {
    # Imposta la CPU sulle massime prestazioni nascondendo l'output
    sudo cpupower frequency-set -g performance > /dev/null
    
    # Esegue il Makefile passando eventuali argomenti (es. "clean")
    make "$@"
    
    # Salva il codice di uscita del processo make
    local MAKE_EXIT=$?
    
    # Ripristina la CPU in modalità risparmio energetico
    sudo cpupower frequency-set -g powersave > /dev/null
    
    # Restituisce al sistema il risultato originale
    return $MAKE_EXIT
}
```
- **Utilizzo base:** Digitare `pmake` al posto di `make` per settare la CPU, compilare, eseguire e rimettere la CPU a riposo in un unico passaggio.
- **Pulizia:** Digitare `pmake clean` per rimuovere file oggetto ed eseguibili dalla directory.

---

## 3. Esecuzione e Statistiche del Benchmark

**Comando di avvio:**
```bash
taskset -c 0 ./bench_pthread --benchmark_repetitions=30 --benchmark_report_aggregates_only=true --benchmark_format=csv > pthread_baseline.csv
```

**Analisi dei flag:**
- `taskset -c 0`: Forza l'utilizzo esclusivo del Core 0.
- `--benchmark_repetitions=30`: Esegue ogni singolo benchmark 30 volte per ridurre il jitter (disturbo) introdotto dall'OS.
- `--benchmark_report_aggregates_only=true`: Nasconde l'output testuale delle singole iterazioni, mostrando esclusivamente le statistiche aggregate.
- `--benchmark_format=csv > pthread_baseline.csv`: Formatta i risultati aggregati in CSV e li reindirizza su file.

---

## 4. Funzionamento Interno del Codice

- **Allocazione:** L'istruzione `->ThreadRange(2, 1024)` genera un vettore di `std::thread`, delegando la creazione all'OS (che richiama internamente `pthread_create`). Nel codice sorgente, questa fase avviene nel file `src/benchmark_runner.cc` ed è incapsulata nel metodo `RunThreads` della classe `ThreadRunnerDefault`.

- **Sincronizzazione alla Barriera:** Per garantire precisione ed escludere i tempi di setup dal timer, i thread vengono bloccati in attesa prima dell'esecuzione. Questa logica di sincronizzazione è implementata nel file `src/thread_manager.h` ed è incapsulata nella classe `ThreadManager`.
    
- **Scopo del test:** Il benchmark non valuta i tempi di creazione o distruzione, ma isola le operazioni dello scheduler nel rilascio volontario della CPU (`sched_yield()`).

- **Avvio della misurazione:** Una volta pronti tutti i thread (entrata nel blocco `for(auto _ : state)`), il thread master verifica che il kernel abbia concluso l'inizializzazione. Successivamente, registra il timestamp, avvia il cronometro e sblocca l'esecuzione parallela dalla barriera.


## 5. Creazione dei Plot dalla lettura dei CSV generati con uno script in python 

**Prerequisiti:**
```bash
pip install pandas matplotlib
```
