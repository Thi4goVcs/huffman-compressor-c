#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "huffman.h"

static unsigned char *ler_arquivo(const char *caminho, size_t *tamanho) {
    FILE *f = fopen(caminho, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (tam < 0) {
        fclose(f);
        return NULL;
    }

    unsigned char *buf = malloc(tam > 0 ? (size_t)tam : 1);
    size_t lidos = tam > 0 ? fread(buf, 1, (size_t)tam, f) : 0;
    fclose(f);

    *tamanho = lidos;
    return buf;
}

static int escrever_arquivo(const char *caminho, const unsigned char *buf, size_t tamanho) {
    FILE *f = fopen(caminho, "wb");
    if (!f) return 0;
    size_t escritos = tamanho > 0 ? fwrite(buf, 1, tamanho, f) : 0;
    fclose(f);
    return escritos == tamanho;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "uso: %s comprimir|descomprimir <entrada> <saida>\n", argv[0]);
        return 1;
    }

    const char *modo = argv[1];
    const char *caminho_entrada = argv[2];
    const char *caminho_saida = argv[3];

    size_t tam_entrada = 0;
    unsigned char *entrada = ler_arquivo(caminho_entrada, &tam_entrada);
    if (!entrada) {
        fprintf(stderr, "erro: nao consegui abrir '%s'\n", caminho_entrada);
        return 1;
    }

    unsigned char *saida = NULL;
    size_t tam_saida = 0;
    int ok;

    if (strcmp(modo, "comprimir") == 0) {
        ok = huffman_comprimir(entrada, tam_entrada, &saida, &tam_saida);
    } else if (strcmp(modo, "descomprimir") == 0) {
        ok = huffman_descomprimir(entrada, tam_entrada, &saida, &tam_saida);
    } else {
        fprintf(stderr, "modo invalido '%s': use comprimir ou descomprimir\n", modo);
        free(entrada);
        return 1;
    }

    free(entrada);

    if (!ok) {
        fprintf(stderr, "erro ao processar '%s' (arquivo corrompido ou invalido?)\n", caminho_entrada);
        return 1;
    }

    if (!escrever_arquivo(caminho_saida, saida, tam_saida)) {
        fprintf(stderr, "erro: nao consegui escrever '%s'\n", caminho_saida);
        free(saida);
        return 1;
    }

    printf("%s (%zu bytes) -> %s (%zu bytes)\n", caminho_entrada, tam_entrada, caminho_saida, tam_saida);

    free(saida);
    return 0;
}
