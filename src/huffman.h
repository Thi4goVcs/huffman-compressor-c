#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stddef.h>

/*
 * Comprime `entrada` (tamanho_entrada bytes) usando codificacao de Huffman
 * por byte. Aloca *saida com malloc (chamador precisa dar free) e escreve
 * o tamanho do buffer comprimido em *tamanho_saida.
 * Retorna 1 em sucesso, 0 se a alocacao falhar.
 */
int huffman_comprimir(const unsigned char *entrada, size_t tamanho_entrada,
                       unsigned char **saida, size_t *tamanho_saida);

/*
 * Descomprime um buffer gerado por huffman_comprimir. Aloca *saida com
 * malloc (chamador precisa dar free) e escreve o tamanho original em
 * *tamanho_saida.
 * Retorna 1 em sucesso, 0 se o cabecalho for invalido ou a alocacao falhar.
 */
int huffman_descomprimir(const unsigned char *entrada, size_t tamanho_entrada,
                          unsigned char **saida, size_t *tamanho_saida);

#endif
