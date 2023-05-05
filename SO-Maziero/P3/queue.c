//-----------------------------------------------------------------------------|
//  CALEBE POMPEO HELPA                                                        |
//  GRR20190359                                                                |
//  SISTEMAS OPERACIONAIS - CARLOS MAZIERO                                     |
//-----------------------------------------------------------------------------|
//  OPERAÇÕES EM UMA FILA GENÉRICA                                             |
//-----------------------------------------------------------------------------|

#include <stdio.h>

#include "queue.h"

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue){

    if(queue == NULL)
        return 0;

    int count = 1;
    queue_t* current = queue->next;
    
    while(current != queue){
        count++;
        current = current->next;
    }
    return count;
}

//------------------------------------------------------------------------------

// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ){

    fprintf(stdout, "%s: [", name);
    if(queue == NULL){
        fprintf(stdout, "]\n");
    }
    else{
        queue_t *first = queue;
        print_elem(first);

        queue_t *current = first->next;
        while(current != first){
            fprintf(stdout, " ");
            print_elem(current);
            current = current->next;
        }
        fprintf(stdout, "]\n");
    }
}

//------------------------------------------------------------------------------

// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_append (queue_t **queue, queue_t *elem){
    
    if(queue == NULL){
        fprintf(stderr, "### ERRO: Tentativa de insersao em fila inexistente\n");
        return 1;
    }
    if(elem == NULL){
        fprintf(stderr, "### ERRO: Tentativa de insercao de elemento inexistente\n");
        return 3;
    }
    if(elem->next != NULL || elem->prev != NULL){
        fprintf(stderr, "### ERRO: Tentativa de insercao de elemento de outra fila\n");
        return 4;
    }

    if(*queue == NULL){
        *queue = elem;
        elem->next = elem;
        elem->prev = elem;
    }
    else{
        queue_t* first = *queue;
        queue_t* last = first->prev;

        first->prev = elem;
        last->next = elem;
        elem->next = first;
        elem->prev = last;
    }
    return 0;
}

//------------------------------------------------------------------------------

// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_remove (queue_t **queue, queue_t *elem){

    if(queue == NULL){
        fprintf(stderr, "### ERRO: Tentativa de remocao de fila inexistente\n");
        return 1;
    }
    if(*queue == NULL){
        fprintf(stderr, "### ERRO: Tentativa de remocao de fila sem nenhum elemento\n");
        return 2;
    }
    if(elem == NULL){
        fprintf(stderr, "### ERRO: Tentativa de remocao de elemento inexistente\n");
        return 3;
    }
    if(elem->next == NULL || elem->prev == NULL){
        fprintf(stderr, "### ERRO: Tentativa de remocao de um elemento inexistente\n");
        return 4;
    }

    int flag = 0;
    queue_t* first = *queue;
    queue_t* current = first;
    if(current == elem){
        if(current->next == current){
            current->next = NULL;
            current->prev = NULL;
            *queue = NULL;
        }
        else{
            *queue = current->next;
            current->next->prev = current->prev;
            current->prev->next = current->next;
            current->next = NULL;
            current->prev = NULL;
        }
    }
    else{
        current = current->next;
        while(current != first && flag != 1){
            if(current == elem)
                flag = 1;
            current = current->next;
        }
        current = current->prev;
        if(flag == 1){
            current->next->prev = current->prev;
            current->prev->next = current->next;
            current->next = NULL;
            current->prev = NULL;
        }
        else{
            fprintf(stderr, "### ERROR: Tentativa de remocao de um elemento que esta em outra fila\n");
            return 5;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------