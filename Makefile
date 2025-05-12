NAME = webserv

CPP = c++

SRC =  src/main.cpp \
	src/config/initConf.cpp src/config/confParser.cpp \
	src/serverSetup/setupServer.cpp \
	src/request/reqHandler.cpp src/request/req.cpp src/request/requestParser.cpp src/request/routes.cpp src/request/cgiHandler.cpp src/request/reqUtils.cpp \
	src/responce/respnce.cpp src/responce/post.cpp src/responce/resUtils.cpp src/responce/deleteMethode.cpp src/responce/getMethode.cpp \
	src/utils/utils.cpp src/utils/signals.cpp src/utils/conf.cpp \

OBJDIR = obj
OBJ = $(SRC:%.cpp=$(OBJDIR)/%.o)

# -Wall -Wextra -Werror

CPPFLAGS = -fsanitize=address -std=c++98

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJ)
	$(CPP) $(CPPFLAGS) $(OBJ) -o $(NAME)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
