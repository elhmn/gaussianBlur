NAME = gbfilter
SRCS = main.cc
INCS = -I ./
CC = g++
FLAGS = -Wall -Werror -Wextra


all: $(SRC)
	$(CC) $(INCS) -o $(NAME) $(FLAGS) $(SRCS)

debug: $(SRC)
	$(CC) -D DEBUG $(INCS) -o $(NAME) $(FLAGS) $(SRCS)

re:all

fclean:
	rm -rf $(NAME)
