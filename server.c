#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_THREAD 200

void error(char *msg)
{
  perror(msg);
  exit(1);
}

struct params
{
  pthread_mutex_t mutex;
  pthread_cond_t done;
  int id;
};

typedef struct params params_t;

int sockfd, *newsockfd, portno, clilen;
char **arraySender, **arrayRecipient, **arrayMessage;
struct sockaddr_in serv_addr, cli_addr;
int n, count, new, temp;
int j = 0, max = MAX_THREAD;

void *work(void *arg)
{
  int w = 0;
  long double download_before, download_after, upload_before, upload_after;
  char *bufferSender, *bufferChoice, *bufferRecipient, *bufferMessage, *bufferTemp, *bufferOut, *bufferNew, *bufferTime;
  struct timeval timestamp;

  // Allocating enough space for local strings.
  bufferSender = (char *)malloc(256 * sizeof(char));
  bufferChoice = (char *)malloc(256 * sizeof(char));
  bufferRecipient = (char *)malloc(256 * sizeof(char));
  bufferMessage = (char *)malloc(256 * sizeof(char));
  bufferTemp = (char *)malloc(256 * sizeof(char));
  bufferOut = (char *)malloc(256 * sizeof(char));
  bufferNew = (char *)malloc(256 * sizeof(char));
  bufferTime = (char *)malloc(256 * sizeof(char));

  pthread_mutex_lock(&(*(params_t *)(arg)).mutex);

  j++;   // Setting sender's id.
  w = j; // Storing id to a local variable to avoid conflict.

  listen(sockfd, 1);
  clilen = sizeof(cli_addr);
  newsockfd[w] = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

  if (newsockfd < 0)
  {
    error("ERROR on accept.");
  }

  // Creating thread for the next connection, when this connection and id are established.
  pthread_mutex_unlock(&(*(params_t *)(arg)).mutex);
  pthread_cond_signal(&(*(params_t *)(arg)).done);
  
  // Reading sender's address.
  bzero(bufferSender, 256);                  // Erasing sender's address buffer.
  n = read(newsockfd[w], bufferSender, 255); // Reading sender's address.

  if (n < 0)
  {
    error("ERROR reading from socket.");
  }

  // Storing sender's address if it isn't stored yet.
  count = 0;
  while (count < max)
  {
    if (strcmp(arraySender[count], bufferSender) != 0)
    {
      strcpy(arraySender[w], bufferSender); // Storing sender's address to the specified array.
      break;
    }
    count++;
  }

  // Outputting to recipient if recipient's address is found and there is a NEW message for it.
  new = 0;
  count = 0;
  while (count < max)
  {
    if ((strcmp(bufferSender, arrayRecipient[count]) == 0)) // Search for all the new messages for the user.
    {
      new++; // Count new messages.
    }
    count++;
  }

  bzero(bufferNew, 256);                   // Erasing new messages buffer.
  sprintf(bufferNew, "%d", new);           // Converting integer to string.
  n = write(newsockfd[w], bufferNew, 255); // Writing new messages count.

  if (n < 0)
  {
    error("ERROR writing on socket.");
  }

  count = 0;
  while (count < max)
  {
    if ((strcmp(bufferSender, arrayRecipient[count]) == 0)) // Search for all the new messages for the user.
    {
      bzero(bufferOut, 256);                   // Erasing output buffer.
      strcpy(bufferOut, arrayMessage[count]);  // Copying string to the output buffer.
      bzero(arrayRecipient[count], 256);       // Erasing old messages.
      bzero(arrayMessage[count], 256);         // Erasing old messages.
      n = write(newsockfd[w], bufferOut, 255); // Writing message to recipient.

      if (n < 0)
      {
        error("ERROR writing on socket.");
      }
    }
    count++;
  }

  // Reading choice.
  bzero(bufferChoice, 256);                  // Erasing sender's address buffer.
  n = read(newsockfd[w], bufferChoice, 255); // Reading sender's address.

  if (n < 0)
  {
    error("ERROR reading from socket.");
  }

  // Reading recipient's address and message if sender wants to send a message.
  if ((strcmp(bufferChoice, "Y") == 0) || strcmp(bufferChoice, "y") == 0)
  {
    // Reading recipient's address.
    bzero(bufferRecipient, 256);                  // Erasing recipient's address buffer.
    n = read(newsockfd[w], bufferRecipient, 255); // Reading recipient's address.

    if (n < 0)
    {
      error("ERROR reading from socket.");
    }

    // Reading message to be sent to the recipient's address.
    bzero(bufferMessage, 256);                  // Erasing message buffer.
    n = read(newsockfd[w], bufferMessage, 255); // Reading sender's message.

    if (n < 0)
    {
      error("ERROR reading from socket.");
    }

    // Storing recipient's address and message.
    if (arrayMessage[w][0] == '\0') // Checking if it is the first message from a user.
    {
      strcpy(arrayRecipient[w], bufferRecipient); // Storing recipient's address to the specified array.
      snprintf(bufferTemp, 256, "%s sent you a message:\n%s\n", arraySender[w], bufferMessage);
      strcpy(arrayMessage[w], bufferTemp); // Storing edited message to the specified array.
    }
    else if (arrayMessage[w][0] != '\0') // Checking if the user has sent more than one messages.
    {
      count = 0;
      while (count < max)
      {
        if (arrayMessage[count][0] == '\0')
        {
          strcpy(arraySender[count], bufferSender);       // Storing sender's address to the specified array.
          strcpy(arrayRecipient[count], bufferRecipient); // Storing recipient's address to the specified array.
          snprintf(bufferTemp, 256, "%s sent you a message:\n%s\n", arraySender[j], bufferMessage);
          strcpy(arrayMessage[count], bufferTemp); // Storing edited message to the specified array.
          break;
        }
        count++;
      }
    }
  }

  // Freeing allocated space for local strings.
  free(bufferSender);
  free(bufferChoice);
  free(bufferRecipient);
  free(bufferMessage);
  free(bufferTemp);
  free(bufferOut);
  free(bufferNew);
  free(bufferTime);

  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
  int i;
  params_t p;
  pthread_t *threads;
  pthread_attr_t pthread_custom_attr;
  pthread_mutex_init(&p.mutex, NULL);
  pthread_cond_init(&p.done, NULL);

  // Allocating enough space for string arrays and sock array.
  newsockfd = malloc(max * sizeof(int *));

  arraySender = malloc(max * sizeof(char *));
  arrayRecipient = malloc(max * sizeof(char *));
  arrayMessage = malloc(max * sizeof(char *));

  for (count = 0; count < max; count++)
  {
    arraySender[count] = malloc(256 * sizeof(char));
    arrayRecipient[count] = malloc(256 * sizeof(char));
    arrayMessage[count] = malloc(256 * sizeof(char));
  }

  // Threading initialization.
  threads = (pthread_t *)malloc(max * sizeof(*threads));
  pthread_attr_init(&pthread_custom_attr);

  // Socket initialization.
  if (argc < 2)
  {
    fprintf(stderr, "ERROR, no port provided.\n");
    exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0)
  {
    error("ERROR opening socket.");
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    error("ERROR on binding.");
  }

  // Start threads.
  for (i = 0; i < max; i++)
  {
    p.id = i;
    pthread_create(&threads[i], &pthread_custom_attr, work, &p);
    pthread_cond_wait(&p.done, &p.mutex);
  }

  // Wait until all are done.
  for (i = 0; i < max; i++)
  {
    pthread_join(threads[i], NULL);
  }

  pthread_mutex_destroy(&p.mutex);
  pthread_cond_destroy(&p.done);

  // Freeing allocated space for sock array and string arrays.
  free(newsockfd);
  free(arraySender);
  free(arrayRecipient);
  free(arrayMessage);

  exit(0);
}
