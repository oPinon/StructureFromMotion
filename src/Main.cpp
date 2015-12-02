
#include "..\test\tests.h"
#include "OpenGL.h"

#include "Mesh.h"
#include "Poisson.h"
#include "PoissonTree.h"

int main() {

	//testPoisson2D("Poisson/bust.jpg", 1);

	/*Mesh mesh = Mesh::loadNVM("armor.nvm");
	mesh.reCenter();
	mesh.autoScale(2.5);
	//OpenGLMain(mesh);

	Poisson3D::Gradient gradient(mesh, 64);
	//gradient.x.display();
	gradient.getDivergence().write("src.float3");
	system("volumetric.bat");
	OpenGLMain(mesh);*/

	cv::Mat src = cv::imread("Poisson/bust.jpg");
	auto features = Poisson2D::simulateKeyPoints(src, 1);
	//cv::imshow("im", imKeys); cv::waitKey();

	Node tree(NULL, { false, false }, 0, 0, src.size().width, src.size().height);
	for (const auto& feat : features) {
		tree.addPoint(&feat, 5);
	}
	tree.computeGradient();
	cv::Mat toShow; src.copyTo(toShow);
	auto startNode = tree
		.children.getValue({ false,true })
		.children.getValue({ false,false })
		;
	//auto l = startNode.getBorderLeaves(0, true);
	auto l = startNode.getNeighbors(0, true);
	l.merge(startNode.getNeighbors(0, false));
	l.merge(startNode.getNeighbors(1, false));
	l.merge(startNode.getNeighbors(1, true));
	cout << l.size() << endl;

	startNode.draw(toShow, cv::Scalar(0, 255, 0), cv::FILLED);
	for (const auto& a : l) {
		a->draw(toShow, cv::Scalar(0, 0, 255), cv::FILLED);
	}
	tree.draw(toShow);

	//toShow = Poisson2D::showKeyPoints(features, toShow);
	cv::Mat gradTree(src.size(), CV_32FC3); gradTree.setTo(cv::Scalar(0, 0, 0));
	tree.fillGradient(gradTree);
	//tree.draw(gradTree);

	cv::imshow("gradient", gradTree);
	imshow("quadTree", toShow); cv::waitKey(1);
	cv::waitKey();
}