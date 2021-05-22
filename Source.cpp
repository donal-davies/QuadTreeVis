#include <SFML/Graphics.hpp>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <random>
#include <thread>
#include "Source.h"

using namespace std;
using namespace sf;

const unsigned int XMAX = 1000;
const unsigned int YMAX = 1000;
const unsigned int ANT_SIZE = 5;

struct Body {
	double angle;
	double velocity;
	int index;
	QuadTree* container;

	Body(double d, double vel, int i) : angle(d), velocity(vel), index(i), container(0) {};
	~Body() {
		//delete object;
	}
};

struct QuadTree {
	unsigned short MAX_DEPTH = 4;
	unsigned short MAX_CAPACITY = 20;

	QuadTree* children[4];
	vector<Body*> data;
	int depth;
	RectangleShape* bounds;
	QuadTree* parent;
	VertexArray* verts;

	QuadTree(int i, RectangleShape* b, QuadTree* p, VertexArray* v) : children(), data(vector<Body*>()), depth(i), bounds(b), parent(p), verts(v) {
		bounds->setFillColor(Color::Transparent);
		bounds->setOutlineColor(Color(255, 255, 255, 100));
		bounds->setOutlineThickness(-1);
	}

	~QuadTree() {
	}

	void compress() {
		if (children[0]) {
			for (int i = 0; i < 4; ++i)if(children[i]->data.size()==0)children[i] = 0;
		}
	}

	void raiseChildren() {

		if (children[0]) {
			for (int i = 0; i < 4; ++i) {
				for (auto iter = children[i]->data.begin(); iter != children[i]->data.end(); ++iter) {
					data.push_back(*iter);
					(*iter)->container = this;
				}
				children[i]->data.clear();
			}
			compress();
		}
	}

	void prune() {
		if (count_data() == 0)compress();
		else if (count_data() < MAX_CAPACITY)raiseChildren();
		if (parent != 0)parent->prune();
	}

	void remove(Body* target) {
		auto iter = target->container->data.begin();
		while (*iter != target)++iter;
		target->container->data.erase(iter);
		//target->container->prune();
		target->container = 0;
	}

	void split() {
		for (int i = 0; i < 4; ++i) {
			children[i] = new QuadTree(depth + 1, produceBounds(i), this, verts);
		}
	}

	int count_data() {
		if (!children[0])return data.size();
		else {
			int out = 0;
			for (int i = 0; i < 4; ++i) out += children[i]->count_data();
			return out;
		}
	}

	void descendChildren() {
		for (auto iter = data.begin(); iter != data.end(); ++iter) {
			insertObject(*iter);
		}
		data.clear();
	}

	void insertObject(Body* target) {
		int quad = (*verts)[target->index].position.x < (bounds->getPosition().x + bounds->getSize().x / 2) ? 0 : 1;

		quad = (*verts)[target->index].position.y < (bounds->getPosition().y + bounds->getSize().y / 2) ? (quad == 0 ? 0 : 1) : (quad == 0 ? 2 : 3);
		
		if (children[quad] == 0) {
			if (data.size() < MAX_CAPACITY || depth == MAX_DEPTH) {
				data.push_back(target);
				target->container = this;
			}
			else {
				split();
				descendChildren();
				children[quad]->insertObject(target);
			}
		}
		else {
			children[quad]->insertObject(target);
		}
	}

	RectangleShape* produceBounds(int q) {
		RectangleShape* newRect = new RectangleShape(Vector2f(bounds->getSize().x / 2.0f, bounds->getSize().y / 2.0f));

		newRect->setPosition(q % 2 == 0 ? bounds->getPosition().x : bounds->getPosition().x + newRect->getSize().x,
			q < 2 ? bounds->getPosition().y : bounds->getPosition().y + newRect->getSize().y);

		return newRect;
	}

	void drawRect(RenderWindow* window) {
		if(bounds)window->draw(*bounds);
		for (int i = 0; i < 4; ++i) {
			if (!children[i])continue;
			children[i]->drawRect(window);
		}
	}
};


void move(float x, float y, int ind, VertexArray* verts) {
	for (int i = 0; i < 4; ++i) {
		(*verts)[ind + i].position += Vector2f(x, y);
	}
}

void updatePosition(Body* object, QuadTree* root, VertexArray* verts) {
	double xMotion = cos(object->angle);
	double yMotion = sin(object->angle);

	if ((*verts)[object->index].position.x + object->velocity * xMotion <= 0 || (*verts)[object->index].position.x + object->velocity * xMotion >= XMAX) {
		//Reflect about x-axis
		object->angle = fmod(M_PI + object->angle * -1, 2 * M_PI);
	}

	if ((*verts)[object->index].position.y + object->velocity * yMotion <= 0 || (*verts)[object->index].position.y + object->velocity * yMotion >= YMAX) {
		//Reflect about y axis
		object->angle = fmod(-1 * object->angle, 2 * M_PI);
	}

	xMotion = cos(object->angle);
	yMotion = sin(object->angle);

	float newX = (*verts)[object->index].position.x + object->velocity * xMotion;
	float newY = (*verts)[object->index].position.y + object->velocity * yMotion;

	if (!object->container)root->insertObject(object);
	else {
		RectangleShape* bounds = object->container->bounds;

		if ((newX < bounds->getPosition().x || newX > bounds->getPosition().x + bounds->getSize().x) || (newY < bounds->getPosition().y || newY > bounds->getPosition().y + bounds->getSize().y)) {
			root->remove(object);

			move(object->velocity * xMotion, object->velocity * yMotion, object->index, verts);
			root->insertObject(object);
			return;
		}
	}

	move(object->velocity * xMotion, object->velocity * yMotion, object->index, verts);
}


int main() {

	RenderWindow window(VideoMode(XMAX, YMAX), "Quads");
	window.setFramerateLimit(60);

	default_random_engine generator;
	normal_distribution<float> dist(XMAX / 2.0, XMAX/10.0);
	normal_distribution<float> dist2(YMAX / 2.0, YMAX/10.0);
	normal_distribution<double> velDist(1.0, 1.0);
	uniform_real_distribution<double> angleDist(0, 2*M_PI);
	normal_distribution<double> angleDist2(M_PI, M_PI / 6);


	vector<Body> objects;
	VertexArray verts(Quads, 0);
	for (int i = 0; i < 5000; ++i) {
		vector<Vertex*> points;

		for (int x = 0; x < 4; ++x) {
			Vertex* vert = new Vertex(Vector2f(x < 2 ? ANT_SIZE : 0, x == 1 || x == 2 ? ANT_SIZE : 0), Color::Green);
			verts.append(*vert);
		}

		Body b = *(new Body(fmod(angleDist(generator) + angleDist2(generator), 2*M_PI), velDist(generator), 4*i));
		
		objects.push_back(b);
		move(dist(generator), dist2(generator), b.index, &verts);
	}

	RectangleShape* rootRect = new RectangleShape(Vector2f(0., 0.));
	rootRect->setSize(Vector2f(XMAX, YMAX));
	QuadTree root = *new QuadTree(0, rootRect, NULL, &verts);

	for (auto iter = objects.begin(); iter != objects.end(); ++iter)root.insertObject(&*iter);

	while (window.isOpen()) {
		Event event;
		while (window.pollEvent(event)) {
			if (event.type == Event::Closed)window.close();
		}

		window.clear();

		for (int i = 0; i < objects.size(); ++i) {
			updatePosition(&objects[i], &root, &verts);
		}

		root.drawRect(&window);
		window.draw(verts);

		window.display();
	}
}