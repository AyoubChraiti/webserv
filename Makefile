NAME = webserv




#brach


CPP = c++

SRC =  src/main.cpp \
	src/config/initConf.cpp src/config/confParser.cpp \
	src/serverSetup/setupServer.cpp \
	src/request/reqHandler.cpp src/request/reqReader.cpp \
	src/responce/respnce.cpp \
	src/utils/utils.cpp src/request/RequestParser.cpp
	
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
