
#include <stdio.h>
#include <stdbool.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#define LARGURA 1920
#define ALTURA 1080

#define MENU 0
#define JOGO 1
#define CONFIGURACOES 2
#define RANKING 3

int main(void)
{
    // =================================
    // VARIAVEIS
    // =================================

    ALLEGRO_DISPLAY* janela = NULL;
    ALLEGRO_BITMAP* bitmap_Tela1 = NULL;

    ALLEGRO_EVENT_QUEUE* fila = NULL;
    ALLEGRO_TIMER* timer = NULL;

    bool executando = true;
    bool redesenhar = true;

    int telaAtual = MENU;

    float mouseX = 0;
    float mouseY = 0;

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

    al_install_mouse();
    al_install_keyboard();

    // =================================
    // CARREGAR BITMAP DA TELA INICIAL
    // =================================

    bitmap_Tela1 = al_load_bitmap("menu.png");

    if (!bitmap_Tela1)
    {
        printf("ERRO: nao foi possivel carregar menu.png!\n");
        printf("Verifique se menu.png esta na pasta Debug.\n");
        return -1;
    }

    printf("Menu carregado com sucesso!\n");

    // =================================
    // CRIAR JANELA
    // =================================

    janela = al_create_display(LARGURA, ALTURA);

    if (!janela)
    {
        printf("Erro ao criar janela!\n");
        al_destroy_bitmap(bitmap_Tela1);
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

        if (timer)
            al_destroy_timer(timer);

        if (fila)
            al_destroy_event_queue(fila);

        al_destroy_bitmap(bitmap_Tela1);
        al_destroy_display(janela);

        return -1;
    }

    al_register_event_source(
        fila,
        al_get_display_event_source(janela)
    );

    al_register_event_source(
        fila,
        al_get_mouse_event_source()
    );

    al_register_event_source(
        fila,
        al_get_keyboard_event_source()
    );

    al_register_event_source(
        fila,
        al_get_timer_event_source(timer)
    );

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
        // CLIQUES DO MENU
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
            }

            // CONFIGURACOES
            else if (mouseX >= 600 && mouseX <= 1320 &&
                mouseY >= 600 && mouseY <= 710)
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
        // TECLA ESC
        // =================================

        if (evento.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            if (evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            {
                if (telaAtual != MENU)
                {
                    telaAtual = MENU;
                }
                else
                {
                    executando = false;
                }
            }
        }

        // Atualizacao do timer
        if (evento.type == ALLEGRO_EVENT_TIMER)
        {
            redesenhar = true;
        }

        // =================================
        // DESENHAR AS TELAS
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
                // DESENHAR BITMAP DA TELA INICIAL

                al_draw_scaled_bitmap(
                    bitmap_Tela1,
                    0, 0,
                    al_get_bitmap_width(bitmap_Tela1),
                    al_get_bitmap_height(bitmap_Tela1),
                    0, 0,
                    LARGURA, ALTURA,
                    0
                );

                // EFEITO AO PASSAR O MOUSE

                if (mouseX >= 650 && mouseX <= 1270 &&
                    mouseY >= 500 && mouseY <= 610)
                {
                    al_draw_filled_rectangle(
                        650, 500, 1270, 610,
                        al_map_rgba(50, 120, 255, 45)
                    );
                }

                else if (mouseX >= 600 && mouseX <= 1320 &&
                    mouseY >= 600 && mouseY <= 710)
                {
                    al_draw_filled_rectangle(
                        600, 600, 1320, 710,
                        al_map_rgba(50, 120, 255, 45)
                    );
                }

                else if (mouseX >= 650 && mouseX <= 1270 &&
                    mouseY >= 700 && mouseY <= 810)
                {
                    al_draw_filled_rectangle(
                        650, 700, 1270, 810,
                        al_map_rgba(50, 120, 255, 45)
                    );
                }

                else if (mouseX >= 650 && mouseX <= 1270 &&
                    mouseY >= 800 && mouseY <= 910)
                {
                    al_draw_filled_rectangle(
                        650, 800, 1270, 910,
                        al_map_rgba(50, 120, 255, 45)
                    );
                }
            }

            // =================================
            // TELA DO JOGO
            // =================================

            else if (telaAtual == JOGO)
            {
                al_clear_to_color(
                    al_map_rgb(0, 0, 30)
                );

                // INTEGRAR AQUI O CODIGO DO JOGO
            }

            // =================================
            // CONFIGURACOES
            // =================================

            else if (telaAtual == CONFIGURACOES)
            {
                al_clear_to_color(
                    al_map_rgb(0, 0, 30)
                );

                // IMPLEMENTAR CONFIGURACOES
            }

            // =================================
            // RANKING
            // =================================

            else if (telaAtual == RANKING)
            {
                al_clear_to_color(
                    al_map_rgb(0, 0, 30)
                );

                // IMPLEMENTAR RANKING
            }

            al_flip_display();
        }
    }

    // =================================
    // LIBERAR MEMORIA
    // =================================

    al_destroy_bitmap(bitmap_Tela1);

    al_destroy_timer(timer);
    al_destroy_event_queue(fila);

    al_destroy_display(janela);

    return 0;
}