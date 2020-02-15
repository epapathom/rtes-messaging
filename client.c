#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(char *msg)
{
  perror(msg);
  exit(0);
}

int main(int argc, char *argv[])
{
  struct sockaddr_in serv_addr;
  struct hostent *server;
  struct timeval timestamp;
  int sockfd, portno, n, new;
  long double download_before, download_after, upload_before, upload_after;
  char buffer[256];

  if (argc < 3)
  {
    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    exit(0);
  }

  portno = atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0)
  {
    error("ERROR opening socket.");
  }

  server = gethostbyname(argv[1]);

  if (server == NULL)
  {
    fprintf(stderr, "ERROR, no such host.\n");
    exit(0);
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    error("ERROR connecting.");
  }
  
  // Outputting address of the user.
  printf("Please enter your address: ");
  bzero(buffer, 256);                        // Erasing buffer.
  fgets(buffer, 255, stdin);                 // Getting input from console.
  strtok(buffer, "\n");                      // Removing new line.
  n = write(sockfd, buffer, strlen(buffer)); // Writing the user's address.

  if (n < 0)
  {
    error("ERROR writing to socket.");
  }

  // Reading new messages count for the user.
  bzero(buffer, 256);            // Erasing buffer.
  n = read(sockfd, buffer, 255); // Reading new messages count.

  if (n < 0)
  {
    error("ERROR reading from socket.");
  }

  new = atoi(buffer); // Converting new messages count from string to integer.

  // Reading and printing all new messages.
  while (new > 0) // While there are new messages.
  {
    bzero(buffer, 256);            // Erasing buffer.
    n = read(sockfd, buffer, 255); // Reading the new messages.

    if (n < 0)
    {
      error("ERROR reading from socket.");
    }

    printf("%s", buffer); // Printing the new messages.

    new--;
  }

  // Asking if the user would like to send a message to someone.
  printf("Would you like to send a message?(Y/N): ");
  bzero(buffer, 256);                        // Erasing buffer.
  fgets(buffer, 255, stdin);                 // Getting input from console.
  strtok(buffer, "\n");                      // Removing new line.
  n = write(sockfd, buffer, strlen(buffer)); // Writing the user's preference.

  if (n < 0)
  {
    error("ERROR writing to socket.");
  }

  // If the user wants to send a message, ask the recipient's address and message.
  if (strcmp(buffer, "Y") == 0 || strcmp(buffer, "y") == 0)
  {
    // Asking for the recipient's address.
    printf("Please enter the recipient's address: ");
    bzero(buffer, 256);                        // Erasing buffer.
    fgets(buffer, 255, stdin);                 // Getting input from console.
    strtok(buffer, "\n");                      // Removing new line.
    n = write(sockfd, buffer, strlen(buffer)); // Writing recipient's address.

    if (n < 0)
    {
      error("ERROR writing to socket.");
    }

    // Asking for the message to be sent.
    printf("Please enter the message: ");
    bzero(buffer, 256);                        // Erasing buffer.
    fgets(buffer, 255, stdin);                 // Getting input from console.
    strtok(buffer, "\n");                      // Removing new line.
    n = write(sockfd, buffer, strlen(buffer)); // Writing message.

    if (n < 0)
    {
      error("ERROR writing to socket");
    }
  }

  return 0;
}
