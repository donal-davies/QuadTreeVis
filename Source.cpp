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

const int XMAX = 800;
const int YMAX = 800;

struct Body {
	double angle;
	double velocity;
	RectangleShape* object;

	Body(double d, double vel, RectangleShape* rect) : angle(d), velocity(vel), object(rect) {};
	~Body() {
		//delete object;
	}
};

struct QuadTree {
	int MAX_DEPTH = 6;
	int MAX_CAPACITY = 20;

	QuadTree* children[4];
	vector<Body*> data;
	int depth;
	RectangleShape* bounds;
	QuadTree* parent;

	QuadTree(int i, RectangleShape* b, QuadTree* p) : children(), data(vector<Body*>()), depth(i), bounds(b), parent(p) {
		bounds->setFillColor(Color::Transparent);
		bounds->setOutlineColor(Color::White);
		bounds->setOutlineThickness(-1);
	}

	~QuadTree() {
	}

	void compress() {
		if (children[0]) {
			for (int i = 0; i < 4; ++i)if (children[i])if(children[i]->data.size()==0)children[i] = 0;
		}
	}

	void raiseChildren() {

		if (children[0]) {
			for (int i = 0; i < 4; ++i) {
				for (auto iter = children[i]->data.begin(); iter != children[i]->data.end(); ++iter) {
					data.push_back(*iter);
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
		QuadTree* targetNode = locate(target);

		auto iter = targetNode->data.begin();
		while (*iter != target)++iter;
		targetNode->data.erase(iter);
		targetNode->prune();
	}

	void split() {
		for (int i = 0; i < 4; ++i) {
			children[i] = new QuadTree(depth + 1, produceBounds(i), this);
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

	QuadTree* locate(Body* target) {
		for (auto iter = data.begin(); iter != data.end(); ++iter) {
			if (*iter == target) {
				return this;
			}
		}

		int quad = target->object->getPosition().x < (bounds->getPosition().x + bounds->getSize().x / 2) ? 0 : 1;

		quad = target->object->getPosition().y < (bounds->getPosition().y + bounds->getSize().y / 2) ? (quad == 0 ? 0 : 1) : (quad == 0 ? 2 : 3);

		if (children[quad] != 0) {
			return children[quad]->locate(target);
		}
		return children[quad];
	}

	void descendChildren() {
		for (auto iter = data.begin(); iter != data.end(); ++iter) {
			insertObject(*iter);
		}
		data.clear();
	}

	void insertObject(Body* target) {
		int quad = target->object->getPosition().x < (bounds->getPosition().x + bounds->getSize().x / 2) ? 0 : 1;

		quad = target->object->getPosition().y < (bounds->getPosition().y + bounds->getSize().y / 2) ? (quad == 0 ? 0 : 1) : (quad == 0 ? 2 : 3);
		
		if (children[quad] == 0) {
			if (data.size() < MAX_CAPACITY || depth == MAX_DEPTH)data.push_back(target);
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
		switch (q) {
		case 0:
			newRect->setPosition(bounds->getPosition().x, bounds->getPosition().y);
			break;
		case 1:
			newRect->setPosition(bounds->getPosition().x + newRect->getSize().x, bounds->getPosition().y);
			break;
		case 2:
			newRect->setPosition(bounds->getPosition().x, bounds->getPosition().y + newRect->getSize().y);
			break;
		case 3:
			newRect->setPosition(bounds->getPosition().x + newRect->getSize().x, bounds->getPosition().y + newRect->getSize().y);
			break;
		}

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

void updatePosition(Body* object, QuadTree* root) {
	double xMotion = cos(object->angle);
	double yMotion = sin(object->angle);

	if (object->object->getPosition().x + object->velocity * xMotion < 0 || object->object->getPosition().x + object->velocity * xMotion > XMAX) {
		//Reflect about x-axis
		object->angle = fmod(M_PI + object->angle * -1, 2 * M_PI);
	}

	if (object->object->getPosition().y + object->velocity * yMotion < 0 || object->object->getPosition().y + object->velocity * yMotion > YMAX) {
		//Reflect about y axis
		object->angle = fmod(-1 * object->angle, 2 * M_PI);
	}

	xMotion = cos(object->angle);
	yMotion = sin(object->angle);

	float newX = object->object->getPosition().x + object->velocity * xMotion;
	float newY = object->object->getPosition().y + object->velocity * yMotion;

	QuadTree* current = root->locate(object);
	if (current != 0) {
		RectangleShape* bounds = current->bounds;

		if ((newX < bounds->getPosition().x || newX > bounds->getPosition().x + bounds->getSize().x) || (newY < bounds->getPosition().y || newY > bounds->getPosition().y + bounds->getSize().y)) {
			root->remove(object);
			//root->prune();

			object->object->setPosition(newX, newY);
			root->insertObject(object);
			return;
		}
	}
	else {
		root->insertObject(object);
	}

	object->object->setPosition(newX, newY);
}

int main() {

	double WINDOW_X = 800;
	double WINDOW_Y = 800;

	RenderWindow window(VideoMode(WINDOW_X, WINDOW_Y), "Quads");
	window.setFramerateLimit(60);

	default_random_engine generator;
	normal_distribution<double> dist(XMAX / 2.0, XMAX/10.0);
	normal_distribution<double> dist2(YMAX / 2.0, YMAX/10.0);
	normal_distribution<double> velDist(1.0, 0.0);
	uniform_real_distribution<double> angleDist(0, 2*M_PI);


	vector<Body> objects;

	for (int i = 0; i < 5000; ++i) {
		RectangleShape* object = new RectangleShape(Vector2f(4, 4));
		object->setPosition(dist(generator), dist2(generator));
		object->setFillColor(Color::Green);
		Body b = *(new Body(angleDist(generator), velDist(generator), object));
		objects.push_back(b);
	}

	RectangleShape* rootRect = new RectangleShape(Vector2f(0., 0.));
	rootRect->setSize(Vector2f(XMAX, YMAX));
	QuadTree root = *new QuadTree(0, rootRect, NULL);

	for (auto iter = objects.begin(); iter != objects.end(); ++iter)root.insertObject(&*iter);

	while (window.isOpen()) {
		Event event;
		while (window.pollEvent(event)) {
			if (event.type == Event::Closed)window.close();
		}

		window.clear();

		for (auto iter = objects.begin(); iter != objects.end(); ++iter) {
			updatePosition(&*iter, &root);
			if (&*iter)window.draw(*(iter->object));
		}
		root.drawRect(&window);

		window.display();
	}
}