#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <nanoflann.hpp>
#include "gi/bdpt.h"

// -------------------------------------
// Photon map using a kd-tree

struct PhotonMap {
    // kdtree accessors
	inline size_t kdtree_get_point_count() const { return photons.size(); }
	inline float kdtree_get_pt(const size_t idx, const size_t dim) const { return photons[idx].hit.P[dim]; }
	template <class BBOX> inline bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }

    // kdtree clear
    inline void clear() {
        photons.clear();
        kd_tree.reset();
    }

    // kdtree build
    inline void build() {
        assert(!photons.empty());
        kd_tree = std::make_shared<kd_tree_t>(3, *this);
        kd_tree->buildIndex();
    }

    /**
     * @brief K nearest neighbour lookup
     *
     * @param pos Query position
     * @param K Maximum number of nearest neighbors to look for
     * @param indices std::vector to be filled inidices into photons vector
     * @param distances std::vector to be filled with SQUARED distances to photons
     *
     * @return SQUARED distance to furthest away element
     */
    inline float knn_lookup(const glm::vec3& pos, size_t K, std::vector<size_t>& indices, std::vector<float>& distances) const {
        assert(!photons.empty());
        indices.resize(K); distances.resize(K);
		const size_t n_photons = kd_tree->knnSearch(&pos[0], K, &indices[0], &distances[0]);
        indices.resize(n_photons); distances.resize(n_photons);
        return n_photons > 0 ? distances[n_photons - 1] : 0.f;
    }

    // data
    using kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<nanoflann::L2_Simple_Adaptor<float, PhotonMap>, PhotonMap, 3, size_t>;
    std::vector<PathVertex> photons;
    std::shared_ptr<kd_tree_t> kd_tree;
};