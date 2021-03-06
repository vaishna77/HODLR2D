// #include "../include/ACA.hpp"
#include "kernel.hpp"
#include "ACA.hpp"

LowRank::LowRank(userkernel* K, int tol_pow, std::vector<int>& row_indices, std::vector<int>& col_indices) {
  this->K					=	K;
  this->tol_ACA = pow(10,-1.0*tol_pow);
  this->row_indices = row_indices;
  this->col_indices = col_indices;
}

void LowRank::maxAbsVector(const Eigen::VectorXd& v, const std::set<int>& allowed_indices,
                              double max, int& index
                             ) {
  std::set<int>::iterator it;
  index = *allowed_indices.begin();
  max   = abs(v(index));

  for(it = allowed_indices.begin(); it != allowed_indices.end(); it++) {
      if(abs(v(*it)) > max) {
          index   =   *it;
          max     =   abs(v(index));
      }
  }
}

void LowRank::ACA_only_nodes(std::vector<int>& row_bases, std::vector<int>& col_bases, int &computed_rank) {
  int col_index;
  int row_index;
  int N1 = row_indices.size();
  int N2 = col_indices.size();
  Eigen::VectorXd row(N2), col(N1), v(N2), u(N1);
  Eigen::VectorXd row_temp, col_temp;
  std::vector<Eigen::VectorXd> Uvec;
  std::vector<Eigen::VectorXd> Vvec;
  computed_rank = 0;
  double max;
  int min = N1;
  if (N1 > N2) {
    min = N2;
  }
  // Ac = Eigen::MatrixXd(N1, min);
  // Ar = Eigen::MatrixXd(min, N2);

  std::set<int> remaining_row_ind;
  std::set<int> remaining_col_ind;

  for(int l = 0; l < N1; l++) {
      remaining_row_ind.insert(l);
  }
  for(int l= 0; l < N2; l++) {
      remaining_col_ind.insert(l);
  }

  int l_local;
  for(l_local = 0; l_local < N1; l_local++) {
    row_index=l_local;
    row = K->getRow(row_indices[row_index], col_indices);
    this->maxAbsVector(row, remaining_col_ind, max, col_index);
    if(abs(row(int(col_index))) > tol_ACA) {
      break;
    }
  }
  if (l_local == N1) {
    // Ac = Ac.block(0,0,N1,computed_rank);
    // Ar = Ar.block(0,0,computed_rank,N2);
    return;
  }
  v=row/row(int(col_index));
  row_bases.push_back(row_index);
  col_bases.push_back(int(col_index));
  col = K->getCol(col_indices[col_index], row_indices);
  u	=	col;
  Uvec.push_back(u);
  Vvec.push_back(v);
  // Ac.col(computed_rank) = col;
  // Ar.row(computed_rank) = row;
  // Ac.col(computed_rank) = u;
  // Ar.row(computed_rank) = v;
  remaining_col_ind.erase(col_index);
  remaining_row_ind.erase(row_index);
  computed_rank = 1;

  double normS	=	0.0;
  double prev_normS = DBL_MAX;
  this->maxAbsVector(col, remaining_row_ind, max, row_index);

  while (abs(prev_normS-normS) >= tol_ACA*prev_normS && computed_rank < min) {
  // while (abs(row(int(col_index))) > tol_ACA && abs(col(int(row_index))) > tol_ACA && abs(prev_normS-normS) >= tol_ACA*prev_normS && computed_rank < min) {
    row_bases.push_back(int(row_index));
    row = K->getRow(row_indices[row_index], col_indices);
    row_temp = row;
    for (int l=0; l<=computed_rank-1; ++l){
      for (size_t s = 0; s < Vvec[l].size(); s++) {
        row(s)	-=	Uvec[l](row_index)*Vvec[l](s);
      }
    }
    this->maxAbsVector(row, remaining_col_ind, max, col_index);
    col_bases.push_back(int(col_index));
    for (size_t l = 0; l < row.size(); l++) {
      v(l) = row(l)/row(int(col_index));
    }
    col = K->getCol(col_indices[col_index], row_indices);
    col_temp = col;
    for (int l=0; l<=computed_rank-1; ++l){
      for (size_t s = 0; s < Uvec[l].size(); s++) {
        col(s)	-=	Vvec[l](col_index)*Uvec[l](s);
      }
    }
    u	=	col;
    if(u.norm()< 1e-16 || v.norm()< 1e-16) {
      row_bases.pop_back();
      col_bases.pop_back();
      break;
    }
    Uvec.push_back(u);
    Vvec.push_back(v);
    // Ac.col(computed_rank) = col_temp; // cur form
    // Ar.row(computed_rank) = row_temp;
    // Ac.col(computed_rank) = u; // UV form
    // Ar.row(computed_rank) = v;

    ++computed_rank;
    remaining_col_ind.erase(col_index);
    remaining_row_ind.erase(row_index);
    if (computed_rank != 2)
      prev_normS = normS;
    normS		=	pow(normS,2)+pow(u.norm()*v.norm(),2);
    for (int l=0; l<=computed_rank-1; ++l){
      normS	+=	2*abs(Uvec[l].dot(u) * Vvec[l].dot(v));
    }
    normS	=	sqrt(normS);
    this->maxAbsVector(col, remaining_row_ind, max, row_index);
  }
  // Ac = Ac.block(0,0,N1,computed_rank);
  // Ar = Ar.block(0,0,computed_rank,N2);
}
