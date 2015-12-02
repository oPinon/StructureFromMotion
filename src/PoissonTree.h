#pragma once

#include <opencv2\opencv.hpp>
#include <vector>
#include <stack>

#include "Poisson.h"

using namespace std;

typedef Poisson2D::Feature Feature;

// values in a multidimensional space (2 directions per dimension)
template<typename T, uint dimensions>
struct Space {
	vector<T> values;
	// indexes in each dimension	
	T& getValue( vector<bool> indexes) {

		if (indexes.size() != dimensions) { // TODO : remove
			cerr << "Error, accessing space in " <<
				dimensions << " dimensions with coordinates in " <<
				indexes.size() << " dimension" << endl; throw 1;
		}
		uint index = 0;
		for (uint i = 0; i < indexes.size(); i++) {
			index = 2 * index + (indexes[indexes.size() - 1 - i] ? 1 : 0);
		}
		return values[index];
	}
	// const version, HACK : don't duplicate code
	const T& getValue(vector<bool> indexes) const {

		if (indexes.size() != dimensions) { // TODO : remove
			cerr << "Error, accessing space in " <<
				dimensions << " dimensions with coordinates in " <<
				indexes.size() << " dimension" << endl; throw 1;
		}
		uint index = 0;
		for (uint i = 0; i < indexes.size(); i++) {
			index = 2 * index + (indexes[indexes.size() - 1 - i] ? 1 : 0);
		}
		return values[index];
	}
};

struct Node {

	float x, y, width, height; // space coordinates
	const Node* parent;
	const vector<bool> index; // index relative to the parent
	Space<Node, 2> children;
	bool isLeaf = true;
	vector<const Feature*> features;
	Space<vector<const Node*>, 2> neighbors;
	Poisson2D::Vec2 gradient;

	Node(const Node* parent, vector<bool> index, float x, float y, float width, float height) :
		parent(parent), index(index), x(x), y(y), width(width), height(height) {}

	void draw(cv::Mat& img, cv::Scalar color = cv::Scalar(128, 0, 0), int thickness = 1) const {
		if (isLeaf) {
			cv::rectangle(img, cv::Rect(x, y, width, height), color, thickness);
		}
		for (const auto& child : children.values) {
			child.draw(img, color, thickness);
			child.draw(img, color, thickness);
		}
	}

	void fillGradient(cv::Mat& img) const {

		if (isLeaf) { // if leaf, fill
			for (uint px = x; px < x + width; px++) {
				for (uint py = y; py < y + height; py++) {
					float r = 0.5 + 0.5 * 10 * gradient.x;
					float g = 0.5 + 0.5 * 10 * gradient.y;
					float b = 0.5;
					((float*)img.data)[3 * (py*img.size().width + px) + 0] = b;
					((float*)img.data)[3 * (py*img.size().width + px) + 1] = g;
					((float*)img.data)[3 * (py*img.size().width + px) + 2] = r;
				}
			}
		}
		for (const auto& child : children.values) {
			child.fillGradient(img);
			child.fillGradient(img);
		}
	}

	// if collision and depthLeft > 0, split into several children
	void addPoint(const Feature* feature, uint depthLeft) {

		// TODO : every point falls into a leaf at depth D ?
		if (depthLeft == 0 || (isLeaf && features.size() == 0)) { // is it a leaf ?
			features.push_back(feature);
		}
		else {
			if (isLeaf) {
				// creating children nodes
				children.values = {
					Node(this, { false,false }, x + 0,y + 0,width / 2,height / 2),
					Node(this, { true,false }, x + width / 2,y + 0,width / 2,height / 2),
					Node(this, { false,true }, x + 0,y + height / 2,width / 2,height / 2),
					Node(this, { true,true }, x + width / 2,y + height / 2,width / 2,height / 2)
				};
				isLeaf = false;
			}
			features.push_back(feature);
			for (const Feature* feat : features) {
				children.getValue({
					feat->position.x > x + width / 2,
					feat->position.y > y + height / 2
				}).addPoint(feat, depthLeft - 1);
			}
			features.clear();
		}
	}

	void computeGradient() {

		if (isLeaf) { // if leaf, compute gradient
			float gradX = 0, gradY = 0;
			for (const Feature* feat : features) {
				gradX += feat->normal.x;
				gradY += feat->normal.y;
			}
			if (features.size() > 0) {
				float factor = width * height * features.size();
				gradX /= factor;
				gradY /= factor;
			}
			gradient = { gradX, gradY };
		}
		for (auto& child : children.values) {
			child.computeGradient();
			child.computeGradient();
		}
	}

	// get border leaf in the given direction of a given dimension
	list<const Node*> getBorderLeaves(uint dimension, bool direction) const {

		if (isLeaf) { return{ this }; }
		else {
			auto list1 = children.getValue({
				dimension == 0 ? direction : false,
				dimension == 1 ? direction : false
			}).getBorderLeaves(dimension, direction);
			auto list2 = children.getValue({
				dimension == 0 ? direction : true,
				dimension == 1 ? direction : true
			}).getBorderLeaves(dimension, direction);
			list1.merge(list2);
			return list1;
		}
	}

	// TODO : simpler ?
	list<const Node*> getBorderLeaves(uint dimension, bool direction, stack<vector<bool>> directions) const {

		if (isLeaf) { return{ this }; }
		else {
			if (directions.size() == 0) {
				return getBorderLeaves(dimension, direction);
			}
			else {
				auto dir = directions.top();
				dir[dimension] = direction;
				directions.pop();
				return children.getValue(dir).getBorderLeaves(dimension, direction, directions);
			}
		}
	}

	list<const Node*> getNeighbors(uint dimension, bool direction, stack<vector<bool>>& directions) const {

		if (parent == NULL) {
			return{}; // Border of the tree -> no neighbors
		}
		vector<bool> targetPos = index;
		if (targetPos[dimension] == direction) { // you face a wall, go up
			directions.push(targetPos);
			return parent->getNeighbors(dimension, direction, directions);
		}
		else { // go in the target direction
			targetPos[dimension] = direction;
			return parent->children.getValue(targetPos).getBorderLeaves(dimension, !direction, directions);
		}
	}

	list<const Node*> getNeighbors(uint dimension, bool direction) const {
		return getNeighbors(dimension, direction, stack<vector<bool>>());
	}

	list<const Node*> getLeaves() const {
		if (isLeaf) { return{ this }; }
		else {
			list<const Node*> dst;
			for (const auto& child : children.values) {
				dst.merge(child.getLeaves());
			}
			return dst;
		}
	}

	void computeNeighbors() {

		if (isLeaf) {

		}
		else {
			for (auto& child : children.values) { child.computeNeighbors(); }
		}
	}
};