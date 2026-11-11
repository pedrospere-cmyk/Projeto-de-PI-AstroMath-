#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#define LARGURA 1920
#define ALTURA 1080

#define MENU 0
#define JOGO 1
#define CONFIGURACOES 2
#define RANKING 3

#define ENEMIES 4
#define SHOT_SPEED 26.0f
#define SHIP_NOSE_X 230.0f
#define ENEMY_X 1500.0f
#define FIRST_LANE_Y 330.0f
#define LANE_GAP 150.0f
#define ALIGNMENT_RANGE 45.0f
#define ALIGNMENT_BONUS 50

typedef struct {
    int answer;
} Enemy;

typedef struct {
    float x, y;
    float vx, vy;
    int target;
    int bonus;
    bool active;
} Shot;

// Sorteia um numero inteiro dentro do intervalo informado.
static int random_between(int low, int high) {
    return low + rand() % (high - low + 1);
}

// Calcula a posicao vertical de cada faixa de inimigo.
static float lane_y(int lane) {
    return FIRST_LANE_Y + lane * LANE_GAP;
}

// Desenha texto e texto formatado com um valor numerico.
static void draw_text(ALLEGRO_FONT* font, ALLEGRO_COLOR color,
    float x, float y, int flags, const char* text) {
    al_draw_text(font, color, x, y, flags, text);
}

static void draw_textf(ALLEGRO_FONT* font, ALLEGRO_COLOR color,
    float x, float y, int flags, const char* format, int value) {
    char text[128];
    snprintf(text, sizeof(text), format, value);
    draw_text(font, color, x, y, flags, text);
}

// Cria um tiro na direcao do inimigo escolhido, se nao houver outro ativo.
static void fire_at(int target, float ship_y, Shot* shot) {
    if (shot->active) return;

    float target_y = lane_y(target);
    float dx = ENEMY_X - SHIP_NOSE_X;
    float dy = target_y - ship_y;
    float distance = sqrtf(dx * dx + dy * dy);
    shot->x = SHIP_NOSE_X;
    shot->y = ship_y;
    shot->vx = SHOT_SPEED * dx / distance;
    shot->vy = SHOT_SPEED * dy / distance;
    shot->target = target;
    shot->bonus = fabsf(dy) <= ALIGNMENT_RANGE ? ALIGNMENT_BONUS : 0;
    shot->active = true;
}

// Sorteia uma conta e retorna o resultado correto.
static int question_result(char question[128]) {
    int result;
    switch (random_between(0, 3)) {
    case 0: { // Equacao do primeiro grau: ax + b = c
        int coefficient = random_between(2, 9);
        int x = random_between(-5, 12);
        int constant = random_between(-12, 18);
        int right_side = coefficient * x + constant;
        snprintf(question, 128, "Resolva: %dx %+d = %d (x = ?)", coefficient, constant, right_side);
        result = x;
        break;
    }
    case 1: { // Equacao do segundo grau com duas raizes inteiras; pede a maior.
        int root1 = random_between(-5, 8);
        int root2 = random_between(-5, 8);
        while (root2 == root1) root2 = random_between(-5, 8);
        int sum = root1 + root2;
        int product = root1 * root2;
        snprintf(question, 128, "Resolva: x^2 %+d x %+d = 0 (maior raiz)", -sum, product);
        result = root1 > root2 ? root1 : root2;
        break;
    }
    case 2: { // Potencia e raiz quadrada exata.
        int root = random_between(2, 15);
        snprintf(question, 128, "Se x^2 = %d e x > 0, quanto vale x?", root * root);
        result = root;
        break;
    }
    default: { // Porcentagem com resultado inteiro.
        int percentages[] = { 10, 20, 25, 50 };
        int percent = percentages[random_between(0, 3)];
        int base;
        if (percent == 10) base = random_between(4, 20) * 10;
        else if (percent == 20) base = random_between(4, 20) * 5;
        else if (percent == 25) base = random_between(4, 20) * 4;
        else base = random_between(4, 20) * 2;
        snprintf(question, 128, "Quanto e %d%% de %d?", percent, base);
        result = percent * base / 100;
        break;
    }
    }
    return result;
}

// Sorteia qual inimigo tem a resposta certa e preenche as outras alternativas.
static void new_question(char question[128], int* correct, Enemy enemies[ENEMIES]) {
    int result = question_result(question);
    *correct = random_between(0, ENEMIES - 1);
    for (int i = 0; i < ENEMIES; ++i) {
        if (i == *correct) {
            enemies[i].answer = result;
        }
        else {
            int candidate;
            bool duplicate;
            do {
                int offset = random_between(1, 12);
                candidate = result + (rand() % 2 ? offset : -offset);
                duplicate = candidate == result;
                for (int j = 0; j < i; ++j) {
                    if (enemies[j].answer == candidate) duplicate = true;
                }
            } while (duplicate);
            enemies[i].answer = candidate;
        }
    }
}

int main(void)
{
    // =================================
    // VARIAVEIS
    // =================================

    ALLEGRO_DISPLAY* janela = NULL;
    ALLEGRO_BITMAP* bitmap_Tela1 = NULL;
    ALLEGRO_FONT* font = NULL;

    ALLEGRO_EVENT_QUEUE* fila = NULL;
    ALLEGRO_TIMER* timer = NULL;

    bool executando = true;
    bool redesenhar = true;

    int telaAtual = MENU;

    float mouseX = 0;
    float mouseY = 0;

    // Variaveis do Jogo (Fase)
    Enemy enemies[ENEMIES];
    Shot shot = { 0 };
    bool keys[ALLEGRO_KEY_MAX] = { false };
    char question[128];
    char scratchpad[16] = "";
    int scratch_length = 0;
    int correct = 0, score = 0, lives = 3;
    float ship_y = ALTURA / 2.0f;
    bool game_over = false;

    // =================================
    // INICIALIZACAO DO ALLEGRO
    // =================================

    if (!al_init())
    {
        printf("Erro ao inicializar Allegro!\n");
        return -1;
    }

    if (!al_init_image_addon())
    {
        printf("Erro no addon de imagem!\n");
        return -1;
    }

    if (!al_init_primitives_addon())
    {
        printf("Erro no addon de primitivas!\n");
        return -1;
    }

    if (!al_init_font_addon() || !al_init_ttf_addon())
    {
        printf("Erro nos addons de fontes!\n");
        return -1;
    }

    al_install_mouse();
    al_install_keyboard();

    srand((unsigned int)time(NULL));

    // =================================
    // CARREGAR RECURSOS (MENU E FONTES)
    // =================================

    bitmap_Tela1 = al_load_bitmap("menu.png");

    if (!bitmap_Tela1)
    {
        printf("ERRO: nao foi possivel carregar menu.png!\n");
        printf("Verifique se menu.png esta na pasta correta.\n");
        return -1;
    }

    font = al_load_ttf_font("C:\\Windows\\Fonts\\arial.ttf", 36, 0);
    if (!font) {
        font = al_create_builtin_font(); // Fallback caso a fonte do Windows nao abra
    }

    // =================================
    // CRIAR JANELA
    // =================================

    janela = al_create_display(LARGURA, ALTURA);

    if (!janela)
    {
        printf("Erro ao criar janela!\n");
        al_destroy_bitmap(bitmap_Tela1);
        if (font) al_destroy_font(font);
        return -1;
    }

    al_set_window_title(janela, "ASTROMATH");

    // =================================
    // FILA DE EVENTOS E TIMER
    // =================================

    fila = al_create_event_queue();
    timer = al_create_timer(1.0 / 60.0);

    if (!fila || !timer)
    {
        printf("Erro ao criar fila ou timer!\n");

        if (timer) al_destroy_timer(timer);
        if (fila) al_destroy_event_queue(fila);
        al_destroy_bitmap(bitmap_Tela1);
        if (font) al_destroy_font(font);
        al_destroy_display(janela);

        return -1;
    }

    al_register_event_source(fila, al_get_display_event_source(janela));
    al_register_event_source(fila, al_get_mouse_event_source());
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_timer_event_source(timer));

    // Inicializa a primeira questao do jogo
    new_question(question, &correct, enemies);

    al_start_timer(timer);

    // =================================
    // LOOP PRINCIPAL
    // =================================

    while (executando)
    {
        ALLEGRO_EVENT evento;
        al_wait_for_event(fila, &evento);

        // Fechar janela
        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        {
            executando = false;
        }

        // Atualizar posicao do mouse
        if (evento.type == ALLEGRO_EVENT_MOUSE_AXES ||
            evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
        {
            mouseX = evento.mouse.x;
            mouseY = evento.mouse.y;
        }

        // =================================
        // EVENTOS DO MENU
        // =================================

        if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN &&
            evento.mouse.button == 1 &&
            telaAtual == MENU)
        {
            // JOGAR
            if (mouseX >= 650 && mouseX <= 1270 &&
                mouseY >= 500 && mouseY <= 610)
            {
                telaAtual = JOGO;
                // Reseta a partida ao entrar no jogo
                score = 0;
                lives = 3;
                game_over = false;
                ship_y = ALTURA / 2.0f;
                shot.active = false;
                scratch_length = 0;
                scratchpad[0] = '\0';
                new_question(question, &correct, enemies);
            }

            // CONFIGURACOES
            else if (mouseX >= 600 && mouseX <= 1320 &&
                mouseX >= 600 && mouseY >= 600 && mouseY <= 710)
            {
                telaAtual = CONFIGURACOES;
            }

            // RANKING
            else if (mouseX >= 650 && mouseX <= 1270 &&
                mouseY >= 700 && mouseY <= 810)
            {
                telaAtual = RANKING;
            }

            // SAIR
            else if (mouseX >= 650 && mouseX <= 1270 &&
                mouseY >= 800 && mouseY <= 910)
            {
                executando = false;
            }
        }

        // =================================
        // EVENTOS DA FASE (JOGO)
        // =================================

        if (telaAtual == JOGO) {
            if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
                int key = evento.keyboard.keycode;
                keys[key] = true;

                if (game_over && key == ALLEGRO_KEY_R) {
                    score = 0;
                    lives = 3;
                    game_over = false;
                    ship_y = ALTURA / 2.0f;
                    shot.active = false;
                    scratch_length = 0;
                    scratchpad[0] = '\0';
                    new_question(question, &correct, enemies);
                    redesenhar = true;
                }
            }

            if (evento.type == ALLEGRO_EVENT_KEY_UP) {
                keys[evento.keyboard.keycode] = false;
            }

            // Rascunho do teclado
            if (evento.type == ALLEGRO_EVENT_KEY_CHAR && !game_over) {
                int key = evento.keyboard.keycode;
                int character = evento.keyboard.unichar;

                if (key == ALLEGRO_KEY_BACKSPACE) {
                    if (scratch_length > 0) {
                        scratchpad[--scratch_length] = '\0';
                        redesenhar = true;
                    }
                }
                else if (key == ALLEGRO_KEY_C) {
                    scratch_length = 0;
                    scratchpad[0] = '\0';
                    redesenhar = true;
                }
                else if (((character >= '0' && character <= '9') ||
                    character == '+' || character == '=' || character == '-' ||
                    character == 'x' || character == 'X') &&
                    scratch_length < (int)sizeof(scratchpad) - 1) {
                    if (character == 'X') character = 'x';
                    scratchpad[scratch_length++] = (char)character;
                    scratchpad[scratch_length] = '\0';
                    redesenhar = true;
                }
            }

            // Clique nos inimigos para disparar
            if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && !game_over && evento.mouse.button == 1) {
                for (int i = 0; i < ENEMIES; ++i) {
                    float y = lane_y(i);
                    if (evento.mouse.x >= 1370 && evento.mouse.x <= 1810 &&
                        evento.mouse.y >= y - 55 && evento.mouse.y <= y + 55) {
                        fire_at(i, ship_y, &shot);
                        redesenhar = true;
                        break;
                    }
                }
            }
        }

        // =================================
        // TECLA ESC (GERENCIAMENTO DE TELAS)
        // =================================

        if (evento.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            if (evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            {
                if (telaAtual != MENU)
                {
                    telaAtual = MENU; // Volta para o menu principal
                }
                else
                {
                    executando = false; // Sai do jogo se estiver no menu
                }
            }
        }

        // Atualizacao do timer
        if (evento.type == ALLEGRO_EVENT_TIMER)
        {
            redesenhar = true;

            // Logica de movimento da fase se estivermos jogando
            if (telaAtual == JOGO && !game_over) {
                if (keys[ALLEGRO_KEY_UP]) ship_y -= 4.0f;
                if (keys[ALLEGRO_KEY_DOWN]) ship_y += 4.0f;
                if (ship_y < 180.0f) ship_y = 180.0f;
                if (ship_y > ALTURA - 150.0f) ship_y = ALTURA - 150.0f;

                if (shot.active) {
                    shot.x += shot.vx;
                    shot.y += shot.vy;
                    float target_y = lane_y(shot.target);
                    float dx = shot.x - ENEMY_X;
                    float dy = shot.y - target_y;
                    if (dx * dx + dy * dy <= 34.0f * 34.0f) {
                        int choice = shot.target;
                        shot.active = false;
                        if (choice == correct) {
                            score += 100 + shot.bonus;
                            scratch_length = 0;
                            scratchpad[0] = '\0';
                            new_question(question, &correct, enemies);
                        }
                        else {
                            --lives;
                            if (lives <= 0) game_over = true;
                        }
                    }
                    else if (shot.x > LARGURA || shot.y < 0 || shot.y > ALTURA) {
                        shot.active = false;
                    }
                }
            }
        }

        // =================================
        // RENDERIZACAO DAS TELAS
        // =================================

        if (redesenhar && al_is_event_queue_empty(fila))
        {
            redesenhar = false;

            al_clear_to_color(al_map_rgb(0, 0, 20));

            // =================================
            // MENU PRINCIPAL
            // =================================

            if (telaAtual == MENU)
            {
                al_draw_scaled_bitmap(
                    bitmap_Tela1,
                    0, 0,
                    al_get_bitmap_width(bitmap_Tela1),
                    al_get_bitmap_height(bitmap_Tela1),
                    0, 0,
                    LARGURA, ALTURA,
                    0
                );

                // EFEITO AO PASSAR O MOUSE NO MENU
                if (mouseX >= 650 && mouseX <= 1270 && mouseY >= 500 && mouseY <= 610)
                {
                    al_draw_filled_rectangle(650, 500, 1270, 610, al_map_rgba(50, 120, 255, 45));
                }
                else if (mouseX >= 600 && mouseX <= 1320 && mouseY >= 600 && mouseY <= 710)
                {
                    al_draw_filled_rectangle(600, 600, 1320, 710, al_map_rgba(50, 120, 255, 45));
                }
                else if (mouseX >= 650 && mouseX <= 1270 && mouseY >= 700 && mouseY <= 810)
                {
                    al_draw_filled_rectangle(650, 700, 1270, 810, al_map_rgba(50, 120, 255, 45));
                }
                else if (mouseX >= 650 && mouseX <= 1270 && mouseY >= 800 && mouseY <= 910)
                {
                    al_draw_filled_rectangle(650, 800, 1270, 910, al_map_rgba(50, 120, 255, 45));
                }
            }

            // =================================
            // TELA DO JOGO (FASE)
            // =================================

            else if (telaAtual == JOGO)
            {
                al_clear_to_color(al_map_rgb(8, 12, 35));
                ALLEGRO_COLOR white = al_map_rgb(240, 245, 255);
                ALLEGRO_COLOR red = al_map_rgb(245, 65, 75);
                ALLEGRO_COLOR cyan = al_map_rgb(70, 220, 255);

                // Header HUD
                draw_textf(font, red, 55, 55, 0, "VIDAS: %d", lives);
                draw_textf(font, cyan, LARGURA - 55, 55, ALLEGRO_ALIGN_RIGHT, "PONTOS: %d", score);
                draw_text(font, white, LARGURA / 2, 65, ALLEGRO_ALIGN_CENTRE, "ASTROMATH");
                draw_text(font, white, LARGURA / 2, 190, ALLEGRO_ALIGN_CENTRE, question);

                // Area para anotar numeros (rascunho)
                al_draw_rounded_rectangle(560, 270, 1360, 360, 18, 18, cyan, 3);
                char scratch_text[64];
                if (scratch_length == 0) {
                    snprintf(scratch_text, sizeof(scratch_text), "RASCUNHO:");
                }
                else {
                    snprintf(scratch_text, sizeof(scratch_text), "RASCUNHO: %s", scratchpad);
                }
                draw_text(font, white, LARGURA / 2, 295, ALLEGRO_ALIGN_CENTRE, scratch_text);

                // Nave
                al_draw_filled_triangle(SHIP_NOSE_X, ship_y, SHIP_NOSE_X - 90, ship_y - 55,
                    SHIP_NOSE_X - 90, ship_y + 55, cyan);
                draw_text(font, white, 160, ship_y + 72, ALLEGRO_ALIGN_CENTRE, "NAVE");

                // Inimigos e respostas
                for (int i = 0; i < ENEMIES; ++i) {
                    float y = lane_y(i);
                    al_draw_filled_rounded_rectangle(1370, y - 55, 1810, y + 55, 22, 22,
                        al_map_rgb(65, 20, 38));
                    al_draw_rounded_rectangle(1370, y - 55, 1810, y + 55, 22, 22, red, 4);
                    al_draw_filled_circle(ENEMY_X, y, 34, red);
                    al_draw_filled_circle(ENEMY_X - 12, y - 8, 7, al_map_rgb(255, 220, 220));
                    char answer[64];
                    snprintf(answer, sizeof(answer), "%d", enemies[i].answer);
                    draw_text(font, white, 1590, y - 18, ALLEGRO_ALIGN_CENTRE, answer);
                }

                // Tiro
                if (shot.active) {
                    al_draw_filled_circle(shot.x, shot.y, 8, al_map_rgb(255, 235, 90));
                    if (shot.bonus > 0) {
                        draw_text(font, cyan, LARGURA / 2, 850, ALLEGRO_ALIGN_CENTRE,
                            "TIRO ALINHADO: +50 SE ACERTAR");
                    }
                }

                // Game Over da fase
                if (game_over) {
                    al_draw_filled_rounded_rectangle(560, 390, 1360, 650, 24, 24, al_map_rgb(16, 20, 48));
                    draw_text(font, red, LARGURA / 2, 425, ALLEGRO_ALIGN_CENTRE, "FIM DE JOGO");
                    draw_textf(font, white, LARGURA / 2, 500, ALLEGRO_ALIGN_CENTRE, "Pontuacao final: %d", score);
                    draw_text(font, white, LARGURA / 2, 570, ALLEGRO_ALIGN_CENTRE, "Pressione R para jogar novamente ou ESC para o Menu");
                }
            }

            // =================================
            // CONFIGURACOES
            // =================================

            else if (telaAtual == CONFIGURACOES)
            {
                al_clear_to_color(al_map_rgb(0, 0, 30));
                draw_text(font, al_map_rgb(255, 255, 255), LARGURA / 2, ALTURA / 2 - 30, ALLEGRO_ALIGN_CENTRE, "TELA DE CONFIGURACOES");
                draw_text(font, al_map_rgb(150, 150, 150), LARGURA / 2, ALTURA / 2 + 30, ALLEGRO_ALIGN_CENTRE, "Pressione ESC para voltar ao menu");
            }

            // =================================
            // RANKING
            // =================================

            else if (telaAtual == RANKING)
            {
                al_clear_to_color(al_map_rgb(0, 0, 30));
                draw_text(font, al_map_rgb(255, 255, 255), LARGURA / 2, ALTURA / 2 - 30, ALLEGRO_ALIGN_CENTRE, "TELA DE RANKING");
                draw_text(font, al_map_rgb(150, 150, 150), LARGURA / 2, ALTURA / 2 + 30, ALLEGRO_ALIGN_CENTRE, "Pressione ESC para voltar ao menu");
            }

            al_flip_display();
        }
    }

    // =================================
    // LIBERAR MEMORIA
    // =================================

    al_destroy_bitmap(bitmap_Tela1);
    if (font) al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);
    al_destroy_display(janela);

    return 0;
}
