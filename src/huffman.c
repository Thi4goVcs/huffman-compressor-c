#include "huffman.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---------- arvore de Huffman ---------- */

typedef struct No {
    int byte;           /* 0..255 numa folha, -1 num no interno */
    uint64_t freq;
    unsigned long seq;  /* ordem de criacao, usada so pra desempatar no heap */
    struct No *esq, *dir;
} No;

static No *no_criar_folha(int byte, uint64_t freq, unsigned long seq) {
    No *no = malloc(sizeof(No));
    no->byte = byte;
    no->freq = freq;
    no->seq = seq;
    no->esq = no->dir = NULL;
    return no;
}

static No *no_criar_interno(No *esq, No *dir, unsigned long seq) {
    No *no = malloc(sizeof(No));
    no->byte = -1;
    no->freq = esq->freq + dir->freq;
    no->seq = seq;
    no->esq = esq;
    no->dir = dir;
    return no;
}

static void liberar_arvore(No *no) {
    if (!no) return;
    liberar_arvore(no->esq);
    liberar_arvore(no->dir);
    free(no);
}

/* ---------- heap minimo (fila de prioridade) ---------- */

typedef struct {
    No **itens;
    size_t tam;
    size_t cap;
} Heap;

static void heap_iniciar(Heap *h, size_t cap) {
    h->itens = malloc(sizeof(No *) * cap);
    h->tam = 0;
    h->cap = cap;
}

/* freq menor primeiro; empate resolvido por ordem de criacao (seq), pra
 * a arvore sair sempre igual dado o mesmo arquivo de entrada. */
static int heap_menor(const No *a, const No *b) {
    if (a->freq != b->freq) return a->freq < b->freq;
    return a->seq < b->seq;
}

static void heap_trocar(Heap *h, size_t i, size_t j) {
    No *tmp = h->itens[i];
    h->itens[i] = h->itens[j];
    h->itens[j] = tmp;
}

static void heap_inserir(Heap *h, No *no) {
    size_t i = h->tam++;
    h->itens[i] = no;
    while (i > 0) {
        size_t pai = (i - 1) / 2;
        if (heap_menor(h->itens[i], h->itens[pai])) {
            heap_trocar(h, i, pai);
            i = pai;
        } else {
            break;
        }
    }
}

static No *heap_remover_min(Heap *h) {
    No *min = h->itens[0];
    h->tam--;
    h->itens[0] = h->itens[h->tam];
    size_t i = 0;
    for (;;) {
        size_t esq = 2 * i + 1, dir = 2 * i + 2, menor = i;
        if (esq < h->tam && heap_menor(h->itens[esq], h->itens[menor])) menor = esq;
        if (dir < h->tam && heap_menor(h->itens[dir], h->itens[menor])) menor = dir;
        if (menor == i) break;
        heap_trocar(h, i, menor);
        i = menor;
    }
    return min;
}

static void heap_destruir(Heap *h) {
    free(h->itens);
}

/* Reconstroi a arvore a partir da tabela de frequencias. Determinista:
 * mesma tabela sempre produz a mesma arvore, o que e o que permite ao
 * descompressor refazer a mesma arvore do compressor so com a tabela. */
static No *construir_arvore(const uint32_t freq[256], size_t *num_simbolos_out) {
    Heap heap;
    heap_iniciar(&heap, 256);
    unsigned long seq = 0;
    size_t num_simbolos = 0;

    for (int b = 0; b < 256; b++) {
        if (freq[b] > 0) {
            heap_inserir(&heap, no_criar_folha(b, freq[b], seq++));
            num_simbolos++;
        }
    }

    if (num_simbolos_out) *num_simbolos_out = num_simbolos;

    if (num_simbolos == 0) {
        heap_destruir(&heap);
        return NULL;
    }

    if (num_simbolos == 1) {
        No *unica = heap_remover_min(&heap);
        heap_destruir(&heap);
        return unica;
    }

    while (heap.tam > 1) {
        No *a = heap_remover_min(&heap);
        No *b = heap_remover_min(&heap);
        heap_inserir(&heap, no_criar_interno(a, b, seq++));
    }

    No *raiz = heap_remover_min(&heap);
    heap_destruir(&heap);
    return raiz;
}

/* ---------- geracao dos codigos (folha -> string de bits) ---------- */

static char *duplicar_string(const char *s) {
    size_t len = strlen(s) + 1;
    char *copia = malloc(len);
    memcpy(copia, s, len);
    return copia;
}

static void gerar_codigos_rec(No *no, char *buffer, int profundidade, char *codigos[256]) {
    if (no->esq == NULL && no->dir == NULL) {
        buffer[profundidade] = '\0';
        codigos[no->byte] = duplicar_string(buffer);
        return;
    }
    buffer[profundidade] = '0';
    gerar_codigos_rec(no->esq, buffer, profundidade + 1, codigos);
    buffer[profundidade] = '1';
    gerar_codigos_rec(no->dir, buffer, profundidade + 1, codigos);
}

static void gerar_codigos(No *raiz, char *codigos[256]) {
    char buffer[257];
    gerar_codigos_rec(raiz, buffer, 0, codigos);
}

/* ---------- escrita/leitura de bits (MSB primeiro) ---------- */

typedef struct {
    unsigned char *buf;
    size_t cap;
    size_t tam;
    unsigned char atual;
    int bits_no_atual;
} EscritorBits;

static void escritor_bits_iniciar(EscritorBits *eb, size_t cap_inicial) {
    if (cap_inicial < 1) cap_inicial = 1;
    eb->buf = malloc(cap_inicial);
    eb->cap = cap_inicial;
    eb->tam = 0;
    eb->atual = 0;
    eb->bits_no_atual = 0;
}

static void escritor_bits_garantir_espaco(EscritorBits *eb, size_t extra) {
    if (eb->tam + extra > eb->cap) {
        size_t nova_cap = eb->cap * 2;
        while (nova_cap < eb->tam + extra) nova_cap *= 2;
        eb->buf = realloc(eb->buf, nova_cap);
        eb->cap = nova_cap;
    }
}

static void escritor_bits_escrever_bit(EscritorBits *eb, int bit) {
    eb->atual = (unsigned char)((eb->atual << 1) | (bit & 1));
    eb->bits_no_atual++;
    if (eb->bits_no_atual == 8) {
        escritor_bits_garantir_espaco(eb, 1);
        eb->buf[eb->tam++] = eb->atual;
        eb->atual = 0;
        eb->bits_no_atual = 0;
    }
}

static void escritor_bits_finalizar(EscritorBits *eb, unsigned char **saida, size_t *tamanho) {
    if (eb->bits_no_atual > 0) {
        eb->atual = (unsigned char)(eb->atual << (8 - eb->bits_no_atual));
        escritor_bits_garantir_espaco(eb, 1);
        eb->buf[eb->tam++] = eb->atual;
        eb->bits_no_atual = 0;
    }
    *saida = eb->buf;
    *tamanho = eb->tam;
}

typedef struct {
    const unsigned char *buf;
    size_t tam;
    size_t pos_byte;
    int pos_bit;
} LeitorBits;

static void leitor_bits_iniciar(LeitorBits *lb, const unsigned char *buf, size_t tam) {
    lb->buf = buf;
    lb->tam = tam;
    lb->pos_byte = 0;
    lb->pos_bit = 0;
}

static int leitor_bits_ler_bit(LeitorBits *lb) {
    if (lb->pos_byte >= lb->tam) return -1;
    int bit = (lb->buf[lb->pos_byte] >> (7 - lb->pos_bit)) & 1;
    lb->pos_bit++;
    if (lb->pos_bit == 8) {
        lb->pos_bit = 0;
        lb->pos_byte++;
    }
    return bit;
}

/* ---------- cabecalho little-endian ---------- */

static void escrever_u32_le(unsigned char *buf, uint32_t valor) {
    buf[0] = (unsigned char)(valor & 0xFF);
    buf[1] = (unsigned char)((valor >> 8) & 0xFF);
    buf[2] = (unsigned char)((valor >> 16) & 0xFF);
    buf[3] = (unsigned char)((valor >> 24) & 0xFF);
}

static uint32_t ler_u32_le(const unsigned char *buf) {
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
}

/* Cabecalho: "HUF1" (4 bytes) + tamanho original (4) + tabela de
 * frequencias, uma entrada uint32 por valor de byte (256 * 4 = 1024). */
#define TAMANHO_MAGIC 4
#define TAMANHO_CABECALHO (TAMANHO_MAGIC + 4 + 256 * 4)

int huffman_comprimir(const unsigned char *entrada, size_t tamanho_entrada,
                       unsigned char **saida, size_t *tamanho_saida) {
    uint32_t freq[256] = {0};
    for (size_t i = 0; i < tamanho_entrada; i++) {
        freq[entrada[i]]++;
    }

    size_t num_simbolos = 0;
    No *raiz = construir_arvore(freq, &num_simbolos);

    EscritorBits eb;
    escritor_bits_iniciar(&eb, tamanho_entrada / 2 + 16);

    if (num_simbolos >= 2) {
        char *codigos[256] = {0};
        gerar_codigos(raiz, codigos);
        for (size_t i = 0; i < tamanho_entrada; i++) {
            const char *codigo = codigos[entrada[i]];
            for (size_t k = 0; codigo[k] != '\0'; k++) {
                escritor_bits_escrever_bit(&eb, codigo[k] - '0');
            }
        }
        for (int b = 0; b < 256; b++) free(codigos[b]);
    }
    /* num_simbolos 0 (entrada vazia) ou 1 (um unico byte repetido) nao
     * precisam de nenhum bit: o tamanho original + a tabela de
     * frequencias ja bastam pra reconstruir a saida na descompressao. */

    liberar_arvore(raiz);

    unsigned char *bits_buf;
    size_t bits_tam;
    escritor_bits_finalizar(&eb, &bits_buf, &bits_tam);

    unsigned char *saida_buf = malloc(TAMANHO_CABECALHO + bits_tam);
    if (!saida_buf) {
        free(bits_buf);
        return 0;
    }

    memcpy(saida_buf, "HUF1", TAMANHO_MAGIC);
    escrever_u32_le(saida_buf + TAMANHO_MAGIC, (uint32_t)tamanho_entrada);
    for (int b = 0; b < 256; b++) {
        escrever_u32_le(saida_buf + TAMANHO_MAGIC + 4 + b * 4, freq[b]);
    }
    memcpy(saida_buf + TAMANHO_CABECALHO, bits_buf, bits_tam);
    free(bits_buf);

    *saida = saida_buf;
    *tamanho_saida = TAMANHO_CABECALHO + bits_tam;
    return 1;
}

int huffman_descomprimir(const unsigned char *entrada, size_t tamanho_entrada,
                          unsigned char **saida, size_t *tamanho_saida) {
    if (tamanho_entrada < TAMANHO_CABECALHO) return 0;
    if (memcmp(entrada, "HUF1", TAMANHO_MAGIC) != 0) return 0;

    uint32_t tamanho_original = ler_u32_le(entrada + TAMANHO_MAGIC);
    uint32_t freq[256];
    for (int b = 0; b < 256; b++) {
        freq[b] = ler_u32_le(entrada + TAMANHO_MAGIC + 4 + b * 4);
    }

    size_t num_simbolos = 0;
    No *raiz = construir_arvore(freq, &num_simbolos);

    unsigned char *saida_buf = malloc(tamanho_original > 0 ? tamanho_original : 1);
    if (!saida_buf) {
        liberar_arvore(raiz);
        return 0;
    }

    int ok = 1;

    if (num_simbolos == 1) {
        for (uint32_t i = 0; i < tamanho_original; i++) {
            saida_buf[i] = (unsigned char)raiz->byte;
        }
    } else if (num_simbolos >= 2) {
        LeitorBits lb;
        leitor_bits_iniciar(&lb, entrada + TAMANHO_CABECALHO, tamanho_entrada - TAMANHO_CABECALHO);
        for (uint32_t i = 0; i < tamanho_original && ok; i++) {
            No *atual = raiz;
            while (atual->esq != NULL || atual->dir != NULL) {
                int bit = leitor_bits_ler_bit(&lb);
                if (bit < 0) {
                    ok = 0;
                    break;
                }
                atual = bit == 0 ? atual->esq : atual->dir;
            }
            if (ok) saida_buf[i] = (unsigned char)atual->byte;
        }
    }
    /* num_simbolos == 0: tamanho_original tambem e 0, saida_buf fica vazio. */

    liberar_arvore(raiz);

    if (!ok) {
        free(saida_buf);
        return 0;
    }

    *saida = saida_buf;
    *tamanho_saida = tamanho_original;
    return 1;
}
