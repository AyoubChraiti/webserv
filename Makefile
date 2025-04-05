NAME = webserv

CPP = c++

SRC =  src/main.cpp \
	src/config/initConf.cpp src/config/confParser.cpp \
	src/serverSetup/setupServer.cpp \
	src/request/reqHandler.cpp src/request/req.cpp src/request/requestParser.cpp \
	src/responce/respnce.cpp src/responce/post.cpp \
	src/utils/utils.cpp src/utils/signals.cpp src/utils/conf.cpp \
	
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