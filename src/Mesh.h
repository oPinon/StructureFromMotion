#include <vector>
#include <queue>

#include <fstream>
#include <sstream>

using namespace std;

typedef unsigned char uchar;
typedef unsigned int uint;

struct Mesh {

	struct Vec3 {
		float x, y, z;
		float dist2(const Vec3& other) const {
			float dx = x - other.x;
			float dy = y - other.y;
			float dz = z - other.z;
			return dx*dx + dy*dy + dz*dz;
		}
		float dist(const Vec3& other) const {
			return sqrt(dist2(other));
		}
		Vec3 normalize() {
			float norm = sqrt(x*x + y*y + z*z);
			return{
				x / norm,
				y / norm,
				z / norm
			};
		}
		Vec3 operator+(const Vec3& v) const {
			return{
				x + v.x,
				y + v.y,
				z + v.z
			};
		}
		Vec3 operator/(float v) const {
			return{
				x / v,
				y / v,
				z / v
			};
		}
	};

	struct Color {
		uchar r = 255, g = 255, b = 255;
	};

	struct Point {
		Vec3 pos;
		Color col;
	};

	struct Triangle {
		uint v0, v1, v2; // index from a vertice array
	};

	vector<Point> points;
	vector<Triangle> triangles;

	// used for rendering
	vector<float> vertices;
	vector<uchar> colors;
	vector<uint> triangleIndexes;

	void reCenter() {

		float cX = 0, cY = 0, cZ = 0;
		for (const auto& pt : points) {
			cX += pt.pos.x;
			cY += pt.pos.y;
			cZ += pt.pos.z;
		}
		cX /= points.size();
		cY /= points.size();
		cZ /= points.size();
		for (auto& pt : points) {
			pt.pos.x -= cX;
			pt.pos.y -= cY;
			pt.pos.z -= cZ;
		}

		for (auto& view : views) {
			view.pos = view.pos + Vec3{ -cX, -cY, -cZ };
		}
	}

	void autoScale() {

		float vX = 0, vY = 0, vZ = 0;
		for (const auto& pt : points) {
			vX += pt.pos.x * pt.pos.x;
			vY += pt.pos.y * pt.pos.y;
			vZ += pt.pos.z * pt.pos.z;
		}
		vX = sqrt(vX / points.size());
		vY = sqrt(vY / points.size());
		vZ = sqrt(vZ / points.size());
		float ratio = sqrt((vX*vX + vY*vY + vZ*vZ) / 3);
		for (auto& pt : points) {
			pt.pos.x /= ratio;
			pt.pos.y /= ratio;
			pt.pos.z /= ratio;
		}
		for (auto& view : views) {
			view.pos = view.pos / ratio;
		}
	}

	struct PointPair1 {
		float dist;
		uint index;
		struct Comparer {
			bool operator()(const PointPair1& a, const PointPair1& b) const {
				return a.dist > b.dist;
			}
		};
	};

	// for each point, find the 2 best neighbors
	// HACK : bad results
	void triangulatePoints(
		float maxDist = 10, // max size of a triangle
		uint maxNeighs = 100 // max number of points to look for (HACK : should be infinity)
		) {
		vector<priority_queue<PointPair1, vector<PointPair1>, PointPair1::Comparer>> neighbors(points.size());
		for (uint i = 0; i < points.size(); i++) { // for each point
			Point& p0 = points[i];
			auto& queue = neighbors[i]; // list of near points
			for (uint j = i + 1; j < points.size(); j++) {
				float dist = p0.pos.dist2(points[j].pos);
				if (dist < maxDist && (queue.size() == 0 || dist < queue.top().dist)) {
					queue.push({ dist, j });
				}
				if (queue.size() > maxNeighs) { queue.pop(); }
			}
		}

		// creating triangles
		for (uint i = 0; i < points.size(); i++) {
			Point& p0 = points[i];
			auto& queue = neighbors[i];
			vector<uint> points2(queue.size());
			for (uint j = 0; j < queue.size(); j++) {
				points2[j] = queue.top().index;
				queue.pop();
			}

			// find the best triangle
			float bestDist = INFINITY;
			uint bestI1 = 0, bestI2 = 0;
			for (uint j = 0; j < points2.size(); j++) {
				uint p1I = points2[j];
				Point& p1 = points[p1I];
				for (uint k = j + 1; k < points2.size(); k++) {
					uint p2I = points2[k];
					Point& p2 = points[p2I];
					float dist = p0.pos.dist2(p1.pos) + p0.pos.dist2(p2.pos) + p1.pos.dist2(p2.pos);
					if (dist < bestDist) {
						bestDist = dist;
						bestI1 = p1I;
						bestI2 = p2I;
					}
				}
			}
			if (bestDist < maxDist) {
				triangles.push_back({ i, bestI1, bestI2 });
			}
		}
		cout << "finished" << endl;
	}

	// pre-computes arrays for OpenGL rendering
	void preRender() {

		// converting points
		vertices = vector<float>(3 * points.size());
		colors = vector<uchar>(3 * points.size());
		for (uint i = 0; i < points.size(); i++) {
			Point& pt = points[i];
			vertices[3 * i + 0] = pt.pos.x;
			vertices[3 * i + 1] = pt.pos.y;
			vertices[3 * i + 2] = pt.pos.z;
			colors[3 * i + 0] = pt.col.r;
			colors[3 * i + 1] = pt.col.g;
			colors[3 * i + 2] = pt.col.b;
		}

		// converting triangles
		triangleIndexes = vector<uint>(3 * triangles.size());
		for (uint i = 0; i < triangles.size(); i++) {
			Triangle& tri = triangles[i];
			triangleIndexes[3 * i + 0] = tri.v0;
			triangleIndexes[3 * i + 1] = tri.v1;
			triangleIndexes[3 * i + 2] = tri.v2;
		}
	}

	struct CameraView {

		struct Feature {
			uint ptIndex;
			uint index;
			float x, y;
		};

		struct Quaternion {
			float w, x, y, z;
			Quaternion operator*(const Quaternion& q) const {
				return{
					w * q.w - x * q.x - y * q.y - z * q.z,
					x * q.w + w * q.x - z * q.y + y * q.z,
					y * q.w + z * q.x + w * q.y - x * q.z,
					z * q.w - y * q.x + x * q.y + w * q.z
				};
			}
			Quaternion conj() const {
				return{
					w,
					-x,
					-y,
					-z
				};
			}
			Vec3 transform(const Vec3& v) const {
				Quaternion dst = Quaternion{ w,-x,-y,-z }*(Quaternion{ 0, v.x, v.y, v.z }*Quaternion{ w, x, y, z });
				return{ dst.x, dst.y, dst.z };
			}
		};

		string imgPath;
		vector<Feature> features;
		Vec3 pos;
		float focal;
		Quaternion orientation;
	};

	vector<CameraView> views;

	// http://ccwu.me/vsfm/doc.html#NVM
	static Mesh loadNVM(string fileName) {

		Mesh mesh;

		// importing points
		ifstream in(fileName);
		string line;
		getline(in, line);
		getline(in, line);
		getline(in, line);

		// views
		int nbViews = stoi(line);
		cout << nbViews << " views" << endl;
		mesh.views = vector<CameraView>(nbViews);
		for (int i = 0; i < nbViews; i++) {
			CameraView& view = mesh.views[i];
			// filePath
			getline(in, view.imgPath, '	'); // weird delimiter (not actually a space)
			in >> view.focal;
			in >> view.orientation.w >> view.orientation.x >> view.orientation.y >> view.orientation.z;
			in >> view.pos.x >> view.pos.y >> view.pos.z;
			getline(in, line);
		}
		getline(in, line);
		getline(in, line);

		// points
		int nbPoints = stoi(line);
		cout << nbPoints << " points" << endl;
		mesh.points = vector<Point>(nbPoints);
		for (uint i = 0; i < nbPoints; i++) {
			Point& pt = mesh.points[i];
			in >> pt.pos.x >> pt.pos.y >> pt.pos.z;
			int r, g, b;
			in >> r >> g >> b;
			pt.col.r = r;
			pt.col.g = g;
			pt.col.b = b;

			int nbMeasurements;
			in >> nbMeasurements;
			for (uint j = 0; j < nbMeasurements; j++) {
				uint imgIndex, featIndex;
				in >> imgIndex >> featIndex;
				float x, y;
				in >> x >> y;
				mesh.views[imgIndex].features.push_back({
					i,
					featIndex,
					x, y
				});
			}
		}

		return mesh;
	}
};