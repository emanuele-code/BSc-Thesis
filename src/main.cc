#include <stdint.h>

#include "thread.hpp"
#include "hal.hpp"


static CondVar puo_produrre;               
static CondVar puo_consumare;             

static volatile uint8_t disponibili = 0;   




template <typename R1, typename R2, typename R3>
struct Slots {
    static void high(uint8_t i) {
        if(i == 0)      R1::high();  // turn on led resource 1 
        else if(i == 1) R2::high(); 
        else            R3::high();
    }

    static void low(uint8_t i) {
        if(i == 0)      R1::low();  // turn off led resource 1 
        else if(i == 1) R2::low();  
        else            R3::low();
    }
};



template <typename PIN_PROD, typename S>
void* produci(void*) {
    uint8_t idx_prod = 0;   

    for(;;) {
        while(disponibili >= 3)           
            thread_wait(&puo_produrre);

        PIN_PROD::high();               // turn on blue led producer             
        S::high(idx_prod);              // turn on i-green led resource         
        thread_sleep(700);                     
        PIN_PROD::low();                // turn off blu led  

        idx_prod = (idx_prod + 1) % 3;  // change resource 
        disponibili++;                      
        thread_signal(&puo_consumare);  // signal availability
        thread_yield();                 // yield cpu
    }
    return nullptr;
}



template <typename PIN_CONS, typename S>
void* consuma(void*) {
    uint8_t idx_cons = 0;

    for(;;) {
        thread_sleep(800);

        while(disponibili == 0) 
            thread_wait(&puo_consumare);

        uint8_t n = (disponibili == 3) ? 3 : 1;   // if 3 resources are available, consume them all

        PIN_CONS::high();                         // turn on red led consumer
        for(uint8_t i = 0; i < n; i++) {          // iterate how many resources to be consumed
            S::low(idx_cons);                     
            idx_cons = (idx_cons + 1) % 3;        
            thread_sleep(100);
        }
        thread_sleep(700);
        PIN_CONS::low();                          // turn off red led

        disponibili -= n;                         // decrease resource
        thread_signal(&puo_produrre);             // signal consumption
        thread_yield();                           // yield cpu
    }
    return nullptr;
}





static Thread<64> produttore, consumatore;   // reserve memory

int main() {
    hal_init();        // hardware abstraction layer, to turn on pin, tick and timer

    DP13::output();    // activate 5V on pins
    DP12::output();
    DP11::output();
    DP10::output();
    DP9::output();

    using Slot = Slots<DP12, DP11, DP10>; 

    thread_create(&produttore,  produci<DP13, Slot>, nullptr);  // evaluate thread tcb and stacks
    thread_create(&consumatore, consuma<DP9,  Slot>, nullptr);


    thread_start();    // start the scheduler with a first thread_yield and make the main the idle thread
    return 0;
}