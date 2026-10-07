#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/huffman.h"

static void round_trip_igual(const unsigned char *original, size_t tamanho) {
    unsigned char *comprimido = NULL;
    size_t tam_comprimido = 0;
    assert(huffman_comprimir(original, tamanho, &comprimido, &tam_comprimido));

    unsigned char *descomprimido = NULL;
    size_t tam_descomprimido = 0;
    assert(huffman_descomprimir(comprimido, tam_comprimido, &descomprimido, &tam_descomprimido));

    assert(tam_descomprimido == tamanho);
    assert(tamanho == 0 || memcmp(original, descomprimido, tamanho) == 0);

    free(comprimido);
    free(descomprimido);
}

static void test_entrada_vazia(void) {
    round_trip_igual(NULL, 0);
}

static void test_um_unico_byte_repetido(void) {
    unsigned char dados[500];
    memset(dados, 'a', sizeof(dados));
    round_trip_igual(dados, sizeof(dados));
}

static void test_dois_simbolos(void) {
    const unsigned char dados[] = "aaaaaaaabbbbbbbbaaaabbbbaaaa";
    round_trip_igual(dados, sizeof(dados) - 1);
}

static void test_texto_com_distribuicao_desigual_comprime(void) {
    /* 'a' domina a contagem, o resto aparece pouco -> deveria render
     * codigos curtos pro 'a' e o resultado final menor que o original. */
    char dados[2000];
    size_t i = 0;
    while (i < 1800) dados[i++] = 'a';
    while (i < 1850) dados[i++] = 'b';
    while (i < 1900) dados[i++] = 'c';
    while (i < 2000) {
        dados[i] = (char)('d' + (i % 20));
        i++;
    }

    unsigned char *comprimido = NULL;
    size_t tam_comprimido = 0;
    assert(huffman_comprimir((unsigned char *)dados, sizeof(dados), &comprimido, &tam_comprimido));
    assert(tam_comprimido < sizeof(dados));

    unsigned char *descomprimido = NULL;
    size_t tam_descomprimido = 0;
    assert(huffman_descomprimir(comprimido, tam_comprimido, &descomprimido, &tam_descomprimido));
    assert(tam_descomprimido == sizeof(dados));
    assert(memcmp(dados, descomprimido, sizeof(dados)) == 0);

    free(comprimido);
    free(descomprimido);
}

static void test_todos_os_256_valores_de_byte(void) {
    unsigned char dados[256];
    for (int i = 0; i < 256; i++) dados[i] = (unsigned char)i;
    round_trip_igual(dados, sizeof(dados));
}

static void test_cabecalho_invalido_e_rejeitado(void) {
    unsigned char lixo[2000] = {0};
    memcpy(lixo, "XXXX", 4);

    unsigned char *saida = NULL;
    size_t tam_saida = 0;
    assert(!huffman_descomprimir(lixo, sizeof(lixo), &saida, &tam_saida));
}

static void test_buffer_curto_demais_e_rejeitado(void) {
    unsigned char curto[10] = {0};
    memcpy(curto, "HUF1", 4);

    unsigned char *saida = NULL;
    size_t tam_saida = 0;
    assert(!huffman_descomprimir(curto, sizeof(curto), &saida, &tam_saida));
}

int main(void) {
    test_entrada_vazia();
    test_um_unico_byte_repetido();
    test_dois_simbolos();
    test_texto_com_distribuicao_desigual_comprime();
    test_todos_os_256_valores_de_byte();
    test_cabecalho_invalido_e_rejeitado();
    test_buffer_curto_demais_e_rejeitado();

    printf("todos os testes passaram\n");
    return 0;
}
