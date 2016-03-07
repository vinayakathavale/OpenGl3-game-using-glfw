all: sample

sample: game.cpp glad.c
	g++ -o  My2D game.cpp glad.c  -L/usr/local/lib -lGLU -lGL -ldrm -lXdamage -lX11-xcb -lxcb-glx -lxcb-dri2 -lxcb-dri3 -lxcb-present -lxcb-sync -lxshmfence -lglfw -lrt -lm -ldl -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -lXext -lXrender -lXfixes -lX11 -lpthread -lxcb -lXau -lXdmcp -lSOIL -lftgl  -I/usr/local/include -I/usr/local/include/freetype2 -L/usr/local/lib

clean: 
	rm My2D
