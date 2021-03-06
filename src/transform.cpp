#include "scan_matching_skeleton/transform.h"
#include <cmath>
#include "ros/ros.h"
#include <Eigen/Geometry>
#include <complex>

using namespace std;


void transformPoints(const vector<Point> &points, Transform &t, vector<Point> &transformed_points) {
    transformed_points.clear();
    for (int i = 0; i < points.size(); i++) {
        transformed_points.push_back(t.apply(points[i]));
        //printf("%f %transformed_points.back().r, transformed_points.back().theta);
    }
}

// returns the largest real root to ax^3 + bx^2 + cx + d = 0
complex<float> get_cubic_root(float a, float b, float c, float d) {
    //std::cout<< "a= " << a<< ";  b= " << b<< ";  c= " << c<< ";  d= " << d<<";"<<std::endl;
    // Reduce to depressed cubic
    float p = c / a - b * b / (3 * a * a);
    float q = 2 * b * b * b / (27 * a * a * a) + d / a - b * c / (3 * a * a);

    // std::cout<<"p = "<<p<<";"<<std::endl;
    // std::cout<<"q = "<<q<<";"<<std::endl;

    complex<float> xi(-.5, sqrt(3) / 2);

    complex<float> inside = sqrt(q * q / 4 + p * p * p / 27);

    complex<float> root;

    for (float k = 0; k < 3; ++k) {
        // get root for 3 possible values of k
        root = -b / (3 * a) + pow(xi, k) * pow(-q / 2.f + inside, 1.f / 3.f) +
               pow(xi, 2.f * k) * pow(-q / 2.f - inside, 1.f / 3.f);
        //std::cout<<"RootTemp: "<< root<<std::endl;
        if (root.imag() != 0) { return root; }
    }

    return root;
}

// returns the largest real root to ax^4 + bx^3 + cx^2 + dx + e = 0
float greatest_real_root(float a, float b, float c, float d, float e) {
    // Written with inspiration from: https://en.wikipedia.org/wiki/Quartic_function#General_formula_for_roots
    //std::cout<< "a= " << a<< ";  b= " << b<< ";  c= " << c<< ";  d= " << d<< ";  e= " << e<<";"<<std::endl;

    // Reduce to depressed Quadratic
    float p = (8 * a * c - 3 * b * b) / (8 * a * a);
    float q = (b * b * b - 4 * a * b * c + 8 * a * a * d) / (8 * a * a * a);
    float r = (-3 * b * b * b * b + 256 * a * a * a * e - 64 * a * a * b * d + 16 * a * b * b * c) /
              (256 * a * a * a * a);

    // std::cout<<"p = "<<p<<";"<<std::endl;
    // std::cout<<"q = "<<q<<";"<<std::endl;
    // std::cout<<"r = "<<r<<";"<<std::endl;

    // Ferrari's Solution for Quartics: 8m^3 + 8pm^2 + (2p^2-8r)m - q^2 = 0
    complex<float> m = get_cubic_root(8, 8 * p, 2 * p * p - 8 * r, -q * q);

    complex<float> root1 = -b / (4 * a) + (sqrt(2.f * m) + sqrt(-(2 * p + 2.f * m + sqrt(2.f) * q / sqrt(m)))) / 2.f;
    complex<float> root2 = -b / (4 * a) + (sqrt(2.f * m) - sqrt(-(2 * p + 2.f * m + sqrt(2.f) * q / sqrt(m)))) / 2.f;
    complex<float> root3 = -b / (4 * a) + (-sqrt(2.f * m) + sqrt(-(2 * p + 2.f * m - sqrt(2.f) * q / sqrt(m)))) / 2.f;
    complex<float> root4 = -b / (4 * a) + (-sqrt(2.f * m) - sqrt(-(2 * p + 2.f * m - sqrt(2.f) * q / sqrt(m)))) / 2.f;

    vector<complex<float>> roots{root1, root2, root3, root4};

    float max_real_root = 0.f;

    for (complex<float> root: roots) {
        if (root.imag() == 0) {
            max_real_root = max(max_real_root, root.real());
        }
//        std::cout << "Max real root:" << max_real_root << std::endl;

        return max_real_root;
    }

}

void updateTransform(vector<Correspondence> &corresponds, Transform &curr_trans) {
    // Written with inspiration from: https://github.com/AndreaCensi/gpc/blob/master/c/gpc.c
    // You can use the helper functions which are defined above for finding roots and transforming points as and when needed.
    // use helper functions and structs in transform.h and correspond.h
    // input : corresponds : a struct vector of Correspondene struct object defined in correspond.
    // input : curr_trans : A Transform object refernece
    // output : update the curr_trans object. Being a call by reference function, Any changes you make to curr_trans will be reflected in the calling function in the scan_match.cpp program/

// You can change the number of iterations here. More the number of iterations, slower will be the convergence but more accurate will be the results. You need to find the right balance.
    int number_iter = 5;

    for (int i = 0; i < number_iter; i++) {
        // Fill in the values for the matrices
        Eigen::Matrix4f M, W;
        Eigen::Vector4f g(4, 1);
        M << 0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0;
        W << 0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1;
        g << 0, 0, 0, 0;


        for (int j = 0; j < corresponds.size(); j++) {
            Eigen::MatrixXf M_i(2, 4);
            Eigen::Matrix2f C_i;
            Eigen::Vector2f pi_i;

            Point *p = corresponds[i].p;

            M_i << 1, 0, p->getX(), -p->getY(),
                    0, 1, p->getY(), p->getX();
            Eigen::Vector2f ni_i = corresponds[i].getNormalNorm();
            pi_i << corresponds[i].getPiVec();
            C_i << ni_i * ni_i.transpose();
            M += M_i.transpose() * C_i * M_i;
            g += -2 * pi_i.transpose() * C_i * M_i;
        }


        // Define sub-matrices A, B, D from M
        Eigen::Matrix2f A, B, D;

        A << M(0, 0), M(0, 1),
                M(1, 0), M(1, 1);
        B << M(0, 2), M(0, 3),
                M(1, 2), M(1, 3);
        D << M(2, 2), M(2, 3),
                M(3, 2), M(3, 3);

        //define S and S_A matrices from the matrices A B and D
        Eigen::Matrix2f S;
        S = D - B.transpose() * A.inverse() * B;
        Eigen::Matrix2f S_A;
        S_A = S.determinant() * S.inverse();


        Eigen::Matrix4f M1;
        Eigen::Matrix4f M2;
        Eigen::Matrix4f M3;
        Eigen::Matrix2f I;
        I << 1, 0,
                0, 1;
        M1 << A.inverse() * B * B.transpose() * A.inverse().transpose(), -A.inverse() * B,
                (-A.inverse() * B).transpose(), I;

        M2 << A.inverse() * B * S_A * B.transpose() * A.inverse().transpose(), -A.inverse() * B * S_A,
                (-A.inverse() * B * S_A).transpose(), S_A;

        M3 << A.inverse() * B * S_A.transpose() * S.adjoint() * B.transpose() * A.inverse().transpose(),
                -A.inverse() * B * S_A.transpose() * S_A,
                (-A.inverse() * B * S_A.transpose() * S_A).transpose(), S_A.transpose() *
                                                                        S_A;

        // find the value of lambda by solving the equation formed. You can use the greatest real root function
        float lambda = 0;

        ROS_INFO("LAMBDA: %f", lambda);
        //find the value of x which is the vector for translation and rotation
        Eigen::Vector4f x;
        x = -(2 * M + 2 * lambda * W.inverse().transpose()) * g;

        // Convert from x to new transform
        float theta = atan2(x(3), x(2));
        curr_trans = Transform(x(0), x(1), theta);
    }
}
