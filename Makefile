NAME = webserv

CPP = c++

SRC =  src/main.cpp src/config.cpp src/utils.cpp src/setupServer.cpp \
	src/configParser.cpp src/request.cpp src/requestReader.cpp
	
OBJ = $(SRC:.cpp=.o)

CPPFLAGS = #-Wall -Wextra -Werror -std=c++98

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<

all: $(NAME)

$(NAME): $(OBJ)
	$(CPP) $(CFLAGS) $(OBJ) -o $(NAME)

clean:
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: clean re all fclean

.SECONDARY: $(OBJ)