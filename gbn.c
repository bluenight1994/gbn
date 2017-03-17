#include "gbn.h"

state_t s;
extern struct sockaddr_in remote;
extern socklen_t ml, rl;

uint16_t checksum(uint16_t *buf, int nwords)
{
	uint32_t sum;

	for (sum = 0; nwords > 0; nwords--)
		sum += *buf++;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return ~sum;
}

ssize_t gbn_send(int sockfd, const void *buf, size_t len, int flags){
	
	/* TODO: Your code here. */

	/* Hint: Check the data length field 'len'.
	 *       If it is > DATALEN, you will have to split the data
	 *       up into multiple packets - you don't have to worry
	 *       about getting more than N * DATALEN.
	 */

	int n_seg = ceil(len / (double)DATALEN), send_bytes = 0;

	int i = 0;
	printf("n_seg: %d\n", n_seg);
	for (i = 0; i < n_seg; i++) {

		gbnhdr t_hdr;
		t_hdr.type = DATA;
		t_hdr.seqnum = s.seq_num + 1;
		t_hdr.checksum = 0;
		t_hdr.datalen = min(len, DATALEN);
		memcpy(t_hdr.data, buf + i * DATALEN, min(len, DATALEN));
		printf("client send data: %s\n", t_hdr.data);
		int t_sum = checksum((uint16_t *) &t_hdr, sizeof(t_hdr) / 2);
		t_hdr.checksum = t_sum;

		int t_send_ret = sendto(sockfd, &t_hdr, sizeof(t_hdr), 0, (struct sockaddr *) &remote, rl);
		printf("sender send %d\n", t_send_ret);
		if (t_send_ret < 0) {
			return -1;
		}
		gbnhdr tmp;
		int t_ret;
		t_ret = recvfrom(sockfd, &tmp, sizeof(tmp), 0, (struct sockaddr *) &remote, &rl);
		printf("signal type: %d\n", tmp.type);
		if (t_ret == -1) {
			return -1;
		}
		s.seq_num++;
		send_bytes += min(len, DATALEN);
		len -= DATALEN;
	}
	printf("data send success and time to close connection\n");
	return send_bytes;
}

ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags){

	/* TODO: Your code here. */
	printf("server start to receive\n");
	gbnhdr tmp;
	socklen_t tt = sizeof(struct sockaddr);
	ssize_t t_recv_ret = recvfrom(sockfd, &tmp, sizeof(tmp), 0, (struct sockaddr *)&remote, &tt);
	rl = tt;
	printf("server receiver %d\n", t_recv_ret);
	if (t_recv_ret < 0) {
		return -1;
	}

	tmp.checksum = 0;
	if (tmp.type == DATA) {
		printf("data length: %d\n", tmp.datalen);
		memcpy(buf, tmp.data, tmp.datalen);
		s.seq_num = tmp.seqnum;

		gbnhdr t_hdr;
		t_hdr.type = DATAACK;
		t_hdr.seqnum = s.seq_num;
		t_hdr.checksum = 0;
		memcpy(buf, tmp.data, tmp.datalen);
		int t_send_ret;
		t_send_ret = sendto(sockfd, &t_hdr, sizeof(t_hdr), 0, (struct sockaddr *)&remote, rl);
		if (t_send_ret < 0) {
			printf("send data ack error\n");
			return -1;
		}
		printf("server receiver %d\n", t_hdr.datalen);
		return tmp.datalen;
	}
	if (tmp.type == FIN) {
		gbnhdr t_hdr;
		t_hdr.type = FINACK;
		t_hdr.seqnum = s.seq_num;
		t_hdr.checksum = 0;
		sendto(sockfd, &t_hdr, sizeof(t_hdr), 0, (struct sockaddr *)&remote, rl);
	}
	return 0;
}

int gbn_close(int sockfd){

	/* TODO: Your code here. */
	if (s.role == 1) {
		gbnhdr t_hdr;
		t_hdr.type = FIN;
		t_hdr.seqnum = 0;
		t_hdr.checksum = 0;
		sendto(sockfd, &t_hdr, sizeof(t_hdr), 0, (struct sockaddr *)&remote, rl);
		printf("sender FIN packet sent\n");
		return(0);
	}
}

int gbn_connect(int sockfd, const struct sockaddr *server, socklen_t socklen){

	/* TODO: Your code here. */
	s.role = 1;

	memcpy(&remote, server, socklen);
	rl = socklen;

	printf("client try to connect server\n");
	gbnhdr t_hdr;
	t_hdr.type = SYN;
	t_hdr.seqnum = s.seq_num;
	t_hdr.checksum = 0;
	int t_send_ret;
	t_send_ret = sendto(sockfd, &t_hdr, sizeof(t_hdr), 0, server, socklen);
	if (t_send_ret < 0) {
		printf("client send syn error\n");
		return -1;
	}
	printf("client send syn success, expect SYNACK\n");
	gbnhdr tmp;
	int t_ret;
	t_ret = recvfrom(sockfd, &tmp, sizeof(tmp), 0, server, &socklen);
	if (t_ret < 0) {
		printf("client recv synack error\n");
	}
	if (tmp.type == SYNACK) {
		printf("client recv synack success\n");
		printf("connection established\n");
	}
	return(0);
}

int gbn_listen(int sockfd, int backlog){

	/* TODO: Your code here. */
	s.seq_num = 0;
	return(0);
}

int gbn_bind(int sockfd, const struct sockaddr *server, socklen_t socklen){

	/* TODO: Your code here. */
	return bind(sockfd, server, socklen);
}	

int gbn_socket(int domain, int type, int protocol){
		
	/*----- Randomizing the seed. This is used by the rand() function -----*/
	srand((unsigned)time(0));
	
	/* TODO: Your code here. */
	int sockfd;
	sockfd = socket(domain, type, protocol);
	return sockfd;
}

int gbn_accept(int sockfd, struct sockaddr *client, socklen_t *socklen){

	/* TODO: Your code here. */
	gbnhdr t_recv_hdr;
	int t_recv_ret;
	t_recv_ret = recvfrom(sockfd, &t_recv_hdr, sizeof(t_recv_hdr), 0, client, socklen);
	if (t_recv_ret < 0) {
		printf("server receive syn error\n");
	}
	printf("fuck error\n");
	memcpy(&remote, client, *socklen);
	rl = *socklen;

	if (t_recv_hdr.type == SYN) {
		printf("server receive syn success, send back synack\n");
		gbnhdr t_hdr;
		t_hdr.type = SYNACK;
		t_hdr.seqnum = 0;
		t_hdr.checksum = 0;
		int t_send_ret;
		t_send_ret = sendto(sockfd, &t_hdr, sizeof(t_hdr), 0, &remote, rl);
		if (t_send_ret < 0) {
			printf("server send synack error\n");
			return (-1);
		}
		printf("server send synack success\n");
		printf("connection established\n");
		s.state = ESTABLISHED;
		return 0;
	}
	return sockfd;
}

ssize_t maybe_sendto(int  s, const void *buf, size_t len, int flags, \
                     const struct sockaddr *to, socklen_t tolen){

	char *buffer = malloc(len);
	memcpy(buffer, buf, len);
	
	
	/*----- Packet not lost -----*/
	if (rand() > LOSS_PROB*RAND_MAX){
		/*----- Packet corrupted -----*/
		if (rand() < CORR_PROB*RAND_MAX){
			
			/*----- Selecting a random byte inside the packet -----*/
			int index = (int)((len-1)*rand()/(RAND_MAX + 1.0));

			/*----- Inverting a bit -----*/
			char c = buffer[index];
			if (c & 0x01)
				c &= 0xFE;
			else
				c |= 0x01;
			buffer[index] = c;
		}

		/*----- Sending the packet -----*/
		int retval = sendto(s, buffer, len, flags, to, tolen);
		free(buffer);
		return retval;
	}
	/*----- Packet lost -----*/
	else
		return(len);  /* Simulate a success */
}