#include <gtk/gtk.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/poll.h>

#define PORT "7777"    
#define MAXDATASIZE 100 
#define nlen 21
#define msglen 64

struct my_rows{
	char name[20];
};

typedef struct
{
     GtkWidget *entry, *textview;
} Widgets;

struct my_rows names[10];
int sockfd, numbytes, course, number_course, per1 = 0;
char buf[MAXDATASIZE], cmd[3], message_send[msglen], message_recv[msglen];
const gchar matrx[] = "000102101112202122";

GtkWidget *contain1, *contain11;
GtkWidget *pl_gr1[3][3];
GtkWidget *pl_gr2[3][3];
GtkWidget *pl_gr_px[3][3];
GtkWidget *pl_gr_str[3];
GtkWidget *window;
GtkWidget *entry;
GtkWidget *contain;
GtkWidget *contain2;
GtkWidget *button;
Widgets *chat;
fd_set master, read_fds;

static void insert_entry_text (GtkButton*, Widgets*);
void callback_pl_b (GtkWidget * widget, gpointer data);
GtkWidget *xpm_label_box(GtkWidget *parent, gchar *xpm_filename);
void callback_jg (GtkWidget * widget, gpointer data);
void pl_cr();
void callback_pl_b1 (GtkWidget * widget, gpointer data);
void connect_to_server();
gint delete_event (GtkWidget * widget, GdkEvent * event, gpointer data);
void crt_main_menu();

/* *
 * добавить картинку
 * */
GtkWidget *xpm_label_box(
	GtkWidget *parent,
	gchar *xpm_filename)
{
	GtkWidget *box1;
	GtkWidget *pixmapwid;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GtkStyle *style;

	box1 = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box1), 2);

	style = gtk_widget_get_style(parent);

	pixmap = gdk_pixmap_create_from_xpm(
		parent->window, &mask,
		&style->bg[GTK_STATE_NORMAL],
		xpm_filename);
	pixmapwid = gtk_pixmap_new (pixmap, mask);

	gtk_box_pack_start (GTK_BOX (box1),
		pixmapwid, FALSE, FALSE, 3);

	gtk_widget_show(pixmapwid);

	return(box1);
}

/* *
 * ф-ия отображения полученного сообщения
 * */
void insert_recv_text(gchar *text) {
	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter;
	char fulltext[70];
	
	sprintf(fulltext, "opponent: %s", text);
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (chat->textview));

	mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &iter, mark);

	if (gtk_text_buffer_get_char_count(buffer))
	gtk_text_buffer_insert (buffer, &iter, "\n", 1);

	gtk_text_buffer_insert (buffer, &iter, fulltext, -1);
}

/* *
 * ф-ия пересылки и отображения набранного сообщения
 * */
static void insert_entry_text (GtkButton *button, Widgets *w)
{
	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter;
	const gchar *text;
	char fulltext[70];
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w->textview));
	text = gtk_entry_get_text (GTK_ENTRY (w->entry));
	
	memset(message_send, 0, msglen);
	message_send[0] = 20;
	strcpy(message_send+4, text);
	
	sprintf(fulltext, "you: %s", message_send + 4);
	if (send(sockfd, message_send, msglen, 0) == -1) {
		perror("send");
	}

	mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &iter, mark);

	if (gtk_text_buffer_get_char_count(buffer))
	gtk_text_buffer_insert (buffer, &iter, "\n", 1);

	gtk_text_buffer_insert (buffer, &iter, fulltext, -1);
	gtk_editable_delete_text(GTK_EDITABLE(w->entry), 0, -1);
}

/* *
 * Изменение книпки при собственном ходе или ходе противника
 * */
void change_butt(int i, int j, int num)
{
	GtkWidget *button;
	
	gtk_widget_destroy (pl_gr2[i][j]);
		
	pl_gr2[i][j] = gtk_hbox_new(0, 0);
	pl_gr_px[i][j] = gtk_hbox_new(0, 0);
	button = gtk_button_new();
			
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
	GTK_SIGNAL_FUNC (callback_pl_b1), (gpointer) (gpointer) (matrx + ((3*i + j) * 2)));
				
	
	if (num == 1) {
		pl_gr_px[i][j] = xpm_label_box(window, "resources/x.png");
	} else {
		pl_gr_px[i][j] = xpm_label_box(window, "resources/o.png");
	}
	gtk_widget_show(pl_gr_px[i][j]);
	gtk_container_add (GTK_CONTAINER (button), pl_gr_px[i][j]);
				
	gtk_box_pack_start(GTK_BOX(pl_gr2[i][j]), button, TRUE, 10, 0);
	gtk_widget_show (button);
	gtk_box_pack_start(GTK_BOX(pl_gr1[i][j]), pl_gr2[i][j], TRUE, 10, 0);
	gtk_widget_show(pl_gr2[i][j]);
}

/* *
 * Функця проверки наличия сообщения при ожидании новой игры
 * */
gboolean funct123(gpointer data)
{
	read_fds = master;
	if (select(sockfd + 1, &read_fds, NULL, NULL, 0) == -1) {
		perror("select");
	}
	
	if (FD_ISSET(sockfd, &read_fds)) {
		if((numbytes = recv(sockfd, message_recv, msglen, 0)) <= 0) {
			perror("recv");
			return FALSE;
		} else {
			gtk_widget_destroy (contain);
			pl_cr();
			return FALSE;
		}
	} else {
		return TRUE;
	}
}

void crt_end_menu(int command)
{
	GtkWidget *label;
	GtkWidget *button;
	if (command == 30 || command == 40) {
		if ((number_course == 1 && command == 30) || (number_course == 2 && command == 40)) {
			label = gtk_label_new("Вы выиграли!");
		} else label = gtk_label_new("Вы проиграли!");
	}
	if (command == 50) {
		label = gtk_label_new("Ничья!");
	}
	if (command == 255) {
		label = gtk_label_new("Нет подходящих игр,\n попробуйте позже.");
	}
	gtk_widget_destroy (GTK_WIDGET(contain));
 	
	contain = gtk_vbox_new(0, 0);
	contain1 = gtk_vbox_new(0, 0);
	contain2 = gtk_vbox_new(0, 0);
	
	gtk_box_pack_start(GTK_BOX(contain1), label, TRUE, 10, 0);
	gtk_widget_show(label);
				
	gtk_container_add (GTK_CONTAINER(contain), contain1);
	
	button = gtk_button_new_with_label ("Main menu");

	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (crt_main_menu), GTK_OBJECT (window));
		
	gtk_box_pack_start(GTK_BOX(contain2), button, TRUE, 10, 10);
	gtk_widget_show (button);
	
	button = gtk_button_new_with_label ("Quit");

	gtk_signal_connect (GTK_OBJECT (button), "clicked",
		GTK_SIGNAL_FUNC (delete_event), GTK_OBJECT (window));
		
	gtk_box_pack_start(GTK_BOX(contain2), button, TRUE, 10, 10);
	gtk_widget_show (button);
	
	
	
	gtk_container_add (GTK_CONTAINER(contain), contain2);

	gtk_container_add (GTK_CONTAINER(window), contain);
	gtk_widget_show(contain);
	gtk_widget_show(contain1);
	gtk_widget_show(contain2);
	
	close(sockfd);
}

/* *
 * ф-ия реагирования на событие ход противника
 * */
gboolean pl_b(gpointer data)
{
	int cmd, ij, i, j;
	struct timeval tv;
	
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	
	read_fds = master;
	
	if (select(sockfd + 1, &read_fds, NULL, NULL, &tv) == -1) {
		perror("select");
	}
	
	memset(message_recv, 0, msglen);
	
	if (FD_ISSET(sockfd, &read_fds)) {
		if((numbytes = recv(sockfd, message_recv, msglen, 0)) <= 0) {
			perror("recv");
			return FALSE;
		} else {
			cmd = message_recv[0];
			if (cmd == 10) {
				//принимаем i и j и изменяем кнопку и numpl
				ij = (int) message_recv[1];
				i = ij / 10;
				j = ij % 10;
					
				change_butt(i, j, 2);
				memset(message_recv, 0, msglen);
				if ((numbytes = recv(sockfd, message_recv, msglen, 0)) == -1) { 
					perror("recv"); 
					exit(1); 
				}
				cmd = message_recv[0];
				if (cmd == 60)
				{
					course = 1;
				} else {
					crt_end_menu(cmd);
					return FALSE;
				}
				return TRUE;
			} else {
				insert_recv_text(message_recv+4);
				memset(message_recv, 0, msglen);
				return TRUE;
			}
		}
	} else {
		return TRUE;
	}
}

/* *
 * пустая функция обратного вызова, для неактивных кнопок
 * */
void callback_pl_b1 (GtkWidget * widget, gpointer data) {

}

/* *
 * ф-ия реагирования на свой ход
 * */
void callback_pl_b (GtkWidget * widget, gpointer data) {
	
	/*******отправка выбранного значения, если норм, то изменить, если нет, то ничего не делать*/
	gchar *name1;
	name1 = (gchar *) data;
	int ij, cmd;
	int i = name1[0] - '0';
	int j = name1[1] - '0';
	
	//проверить numpl
	//
	//отправить сообщение о i и j 
	//получить подтверждение
	//изменить кнопку
	//запустить таймер ожидания изменения кнопки противника
	
	if (course == 1) {
		memset(message_send, 0, msglen);
		ij = i * 10 + j;
		message_send[0] = (char) 10;
		message_send[1] = (char) ij;
		if (send(sockfd, message_send, msglen, 0) == -1) {
			perror("send");
		}
		memset(message_recv, 0, msglen);
		
		if ((numbytes = recv(sockfd, message_recv, msglen, 0)) == -1) { 
			perror("recv"); 
			exit(1); 
		}
		change_butt(i, j, 1);
		
		cmd = message_recv[0];
		if (cmd == 60)
		{
			course = 2;
		} else {
			crt_end_menu(cmd);
		}
	}
}

/* *
 * создание игрового поля
 * */
void pl_cr()
{
	int i, j;
	GtkWidget *button;
	GtkWidget *table;
	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter;
	GtkWidget *scrolled_win;
	char txt[] = "Chat:\n";
	
	chat = g_slice_new (Widgets);
	contain11 = gtk_hbox_new(0, 0);
	contain = gtk_hbox_new(0, 0);
	table = gtk_table_new (5, 8, TRUE);

	
	for(i = 0; i < 3; i++) {
		pl_gr_str[i]= gtk_vbox_new(0, 0);
		gtk_box_pack_start(GTK_BOX(contain11), pl_gr_str[i], TRUE, 10, 0);
		gtk_widget_show(pl_gr_str[i]);
	}
	
	for(i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			pl_gr1[i][j] = gtk_hbox_new(0, 0);
			gtk_box_pack_start(GTK_BOX(pl_gr_str[i]), pl_gr1[i][j], TRUE, 10, 0);
			gtk_widget_show(pl_gr1[i][j]);
		}
	}
	for(i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			pl_gr2[i][j] = gtk_hbox_new(0, 0);

			button = gtk_button_new();
			gtk_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (callback_pl_b), (gpointer) (matrx + ((3*i + j) * 2)));
			
			pl_gr_px[i][j] = xpm_label_box(window, "resources/zero.png");
			gtk_widget_show(pl_gr_px[i][j]);
			gtk_container_add (GTK_CONTAINER (button), pl_gr_px[i][j]);
			
			gtk_box_pack_start(GTK_BOX(pl_gr2[i][j]), button, TRUE, 10, 0);
			gtk_widget_show (button);
			gtk_box_pack_start(GTK_BOX(pl_gr1[i][j]), pl_gr2[i][j], TRUE, 10, 0);
			gtk_widget_show(pl_gr2[i][j]);
		}
	}
	gtk_table_attach_defaults (GTK_TABLE (table), contain11, 0, 3, 0, 3);
	gtk_widget_show(contain11);
	gtk_widget_show(contain);
	
	button = gtk_button_new_with_label ("Send");
		
	g_signal_connect (G_OBJECT (button), "clicked",
          G_CALLBACK (insert_entry_text), (gpointer) chat);
    g_signal_connect (G_OBJECT (button), "activate",
          G_CALLBACK (insert_entry_text), (gpointer) chat);
          
	gtk_table_attach_defaults (GTK_TABLE (table), button, 6, 8, 4, 5);
	gtk_widget_show(button);
	
	{
	
	chat->textview = gtk_text_view_new ();
    chat->entry = gtk_entry_new ();
	
	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled_win), chat->textview);
	
	gtk_table_attach_defaults (GTK_TABLE (table), scrolled_win, 4, 8, 0, 3);
	
	gtk_table_attach_defaults (GTK_TABLE (table), chat->entry, 0, 5, 4, 5);
	gtk_widget_show(chat->entry);
	gtk_widget_show(scrolled_win);
	gtk_widget_show(chat->textview);
	}

	gtk_container_add (GTK_CONTAINER (contain), table);
	gtk_container_add (GTK_CONTAINER (window), contain);
	gtk_widget_show(table);
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (chat->textview));
	mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &iter, mark);

	gtk_text_buffer_insert (buffer, &iter, txt, -1);
	
	gtk_widget_set_state(chat->textview, GTK_STATE_INSENSITIVE);
	
	g_timeout_add_seconds(1, pl_b, NULL);
	
}

/* *
 * отправить свое имя и присоедениться
 * */
void callback_connect (GtkWidget * widget, gpointer data)
{
	GtkWidget *label;
 	contain11 = gtk_vbox_new(0, 0);
 	const gchar *name;
 	
 	connect_to_server();
 	
 	name = gtk_entry_get_text(GTK_ENTRY(entry));
 	
 	if ((numbytes = recv(sockfd, message_recv, msglen, 0)) == -1) { 
		perror("recv"); 
		exit(1); 
	}
 	
 	memset(message_send, 0, msglen);
 	message_send[0] = 1;
 	course = 1;
 	number_course = 1;
 	
 	strcpy(message_send + 4, name);
 	
 	if (send(sockfd, message_send, msglen, 0) == -1) {
		perror("send");
	}
	
	gtk_widget_destroy (GTK_WIDGET(data));
	
	label = gtk_label_new("Opponent waiting...");
	gtk_box_pack_start(GTK_BOX(contain11), label, TRUE, 10, 0);
	gtk_widget_show(label);
	
	gtk_box_pack_start(GTK_BOX(contain1), contain11, TRUE, 10, 10);
	gtk_widget_show(contain11);
	
	g_timeout_add_seconds(1, funct123, NULL);
}

/* *
 * ф-ия реагирования на событие "Новая игра"
 * */
void callback_enter_name (GtkWidget * widget, gpointer data)
{
 	gtk_widget_destroy (GTK_WIDGET(data));
 	
 	GtkWidget *label;
	GtkWidget *button;
 	
 	contain11 = gtk_vbox_new(0, 0);
 	
 	button = gtk_button_new_with_mnemonic("_Connect");
 	entry = gtk_entry_new();
	
	
	label = gtk_label_new("Enter your name:");
	gtk_box_pack_start(GTK_BOX(contain11), label, TRUE, 10, 0);
	gtk_widget_show(label);
	
	g_signal_connect(button, "clicked", G_CALLBACK(callback_connect), contain11);
	gtk_box_pack_start(GTK_BOX(contain11), entry, TRUE, 10, 0);
	gtk_box_pack_start(GTK_BOX(contain11), button, TRUE, 10, 0);
	gtk_widget_show(entry);
	gtk_widget_show(button);
	
	gtk_box_pack_start(GTK_BOX(contain1), contain11, TRUE, 10, 10);
	gtk_widget_show(contain11);
	
}

/* *
 * ф-ия реагирования на событие выбора аппонента из списка
 * */
void callback_op (GtkWidget * widget, gpointer data) {
	gtk_widget_destroy (GTK_WIDGET(contain));
	memset(message_send, 0, msglen);
	message_send[0] = 2;
	strcpy(message_send + 4, (char *) data);
	
	if (send(sockfd, message_send, msglen, 0) == -1) {
		perror("send");
	}
	pl_cr();
}

/* *
 * ф-ия чтения списка игроков
 * */
void read_list(int count)
{
	int i;
	
	for(i = 0; i < count; i++) {
		if ((numbytes = recv(sockfd, names[i].name, nlen - 1, 0)) == -1) { 
			perror("recv"); 
			exit(1); 
		}
		names[i].name[numbytes] = '\0';
		if (send(sockfd, message_send, msglen, 0) == -1) {
			perror("send");
		}
	}
}
/* *
 * ф-ия реагирования на событие "Присоедениться к игре"
 * */
void callback_jg (GtkWidget * widget, gpointer data)
{
	connect_to_server();
	
 	gtk_widget_destroy (GTK_WIDGET(data));
 	
 	int i, count = 0;
 	GtkWidget *button;
 	GtkWidget *label;
 	contain11 = gtk_vbox_new(0, 0);
 	
 	
 	if ((numbytes = recv(sockfd, message_recv, msglen, 0)) == -1) { 
		perror("recv"); 
		exit(1); 
	}
 	
 	memset(message_send, 0, msglen);
 	memset(message_recv, 0, msglen);
 	
 	message_send[0] = 2;
 	course = 2;
 	number_course = 2;
 	
 	if (send(sockfd, message_send, msglen, 0) == -1) {
		perror("send");
	}
	
 	label = gtk_label_new("List of games:");
	gtk_box_pack_start(GTK_BOX(contain11), label, TRUE, 10, 0);
	gtk_widget_show(label);
	
 	if ((numbytes = recv(sockfd, message_recv, msglen, 0)) == -1) { 
		perror("recv"); 
		exit(1); 
	}
	count = message_recv[1];
	
	if (count != 255)
	{
		read_list(count);
		for(i = 0; i < count; i++) {
			button = gtk_button_new_with_label (names[i].name);

			gtk_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (callback_op), (gpointer) names[i].name);
			gtk_box_pack_start(GTK_BOX(contain11), button, TRUE, 10, 0);
			gtk_widget_show (button);
		}
		gtk_box_pack_start(GTK_BOX(contain1), contain11, TRUE, 10, 10);
		gtk_widget_show(contain11);
	} else {
		crt_end_menu(255);
	}
}

/* *
 * получить адрес
 * */
void *get_in_addr(struct sockaddr *sa) 
{ 
    if (sa->sa_family == AF_INET) { 
        return &(((struct sockaddr_in*)sa)->sin_addr); 
	}

    return &(((struct sockaddr_in6*)sa)->sin6_addr); 
}

/* *
 * удаление
 * */
gint delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
	gtk_main_quit ();
	return (FALSE);
}

/* *
 * ф-ия соединения с сервером
 * */
void connect_to_server()
{
    struct addrinfo hints, *servinfo, *p; 
    int rv; 
    char s[INET6_ADDRSTRLEN];


    memset(&hints, 0, sizeof hints); 
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM; 
    if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0) { 
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); 
        exit(1); 
	}

    for(p = servinfo; p != NULL; p = p->ai_next) { 
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) { 
			perror("client: socket"); 
			continue; 
		}
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) { 
            close(sockfd); 
            perror("client: connect"); 
			continue; 
		}
		break;
	}

    if (p == NULL) { 
        fprintf(stderr, "client: failed to connect\n"); 
        exit(2);
	}

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s); 

	FD_SET(sockfd, &master);
	
    freeaddrinfo(servinfo);
}

/* *
 * ф-ия создания главного меню
 * */
void crt_main_menu()
{
	if (per1 == 1) {
		gtk_widget_destroy (contain);
	} else {
		per1 = 1;
	}
	
	contain = gtk_vbox_new(0, 0);
	contain1 = gtk_vbox_new(0, 0);
	contain11 = gtk_vbox_new(0, 0);
	contain2 = gtk_vbox_new(0, 0);
	
	//Новая игр	
	{
		button = gtk_button_new_with_label ("New game");

		gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (callback_enter_name), (gpointer) contain11);
		
		gtk_box_pack_start(GTK_BOX(contain11), button, FALSE, TRUE, 0);
		gtk_widget_show (button);
	}
	//Присоедениться к игре
	{
		button = gtk_button_new_with_label ("Join game");

		gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (callback_jg), (gpointer) contain11);
		
		gtk_box_pack_start(GTK_BOX(contain11), button, FALSE, TRUE, 0);
		gtk_widget_show (button);
	}
	gtk_box_pack_start(GTK_BOX(contain1), contain11, FALSE, TRUE, 0);
	gtk_widget_show (contain11);
	//Выход
	{
		button = gtk_button_new_with_label ("Quit");

		gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (delete_event), GTK_OBJECT (window));
		
		gtk_box_pack_start(GTK_BOX(contain2), button, FALSE, TRUE, 10);
		gtk_widget_show (button);
	}
	gtk_box_pack_start(GTK_BOX(contain), contain1, FALSE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(contain), contain2, FALSE, TRUE, 10);
	gtk_widget_show (contain1);
	gtk_widget_show (contain2);
	
	gtk_container_add (GTK_CONTAINER(window), contain);
	
	
	gtk_widget_show (contain);
	gtk_widget_show (window);
	
}

int main (int argc, char *argv[])
{
	gtk_init (&argc, &argv);
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	
	gtk_window_set_title (GTK_WINDOW (window), "Krestiki noliki");
	gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		GTK_SIGNAL_FUNC (delete_event), NULL);
	gtk_container_set_border_width (GTK_CONTAINER (window), 20);
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 450);
	
	crt_main_menu();
	
	gtk_main ();
	return 0;
}
