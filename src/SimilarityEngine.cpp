#include "SimilarityEngine.h"

// DISTANCIA ENTRE DUAS FEATURES
float SimilarityEngine::distance(const Features & a, const Features & b) {
	// normaliza cada feature para [0,1] antes de comparar
	float lumDiff = (a.meanLuminance - b.meanLuminance) / 255.0f;
	float varDiff = (a.luminanceVariance - b.luminanceVariance) / 10000.0f;
	float edgeDiff = (a.edgeDensity - b.edgeDensity) / 1.0f;
	float kpDiff = (a.numKeypoints - b.numKeypoints) / 500.0f;
	float texDiff = (a.texture - b.texture) / 500.0f;

	// distancia euclidiana ponderada
	float d = sqrt(
		weightLuminance * lumDiff * lumDiff + weightVariance * varDiff * varDiff + weightEdges * edgeDiff * edgeDiff + weightKeypoints * kpDiff * kpDiff + weightTexture * texDiff * texDiff);

	return d;
}

// ENCONTRA OS N MAIS SIMILARES
vector<SimilarityResult> SimilarityEngine::findSimilar(int index, vector<Features> & allFeatures, int topN) {
	vector<SimilarityResult> results;

	for (int i = 0; i < (int)allFeatures.size(); i++) {
		if (i == index) continue;
		SimilarityResult r;
		r.index = i;
		r.distance = distance(allFeatures[index], allFeatures[i]);
		results.push_back(r);
	}

	// ordena por distancia (mais proximo primeiro)
	sort(results.begin(), results.end(), [](const SimilarityResult & a, const SimilarityResult & b) {
		return a.distance < b.distance;
	});

	// retorna apenas os topN
	if ((int)results.size() > topN) {
		results.resize(topN);
	}

	return results;
}

// AGRUPA POR SIMILARIDADE
vector<vector<int>> SimilarityEngine::groupBySimilarity(vector<Features> & allFeatures, float threshold) {
	int n = allFeatures.size();
	vector<bool> assigned(n, false);
	vector<vector<int>> groups;

	for (int i = 0; i < n; i++) {
		if (assigned[i]) continue;

		vector<int> group;
		group.push_back(i);
		assigned[i] = true;

		for (int j = i + 1; j < n; j++) {
			if (assigned[j]) continue;
			float d = distance(allFeatures[i], allFeatures[j]);
			if (d < threshold) {
				group.push_back(j);
				assigned[j] = true;
			}
		}

		groups.push_back(group);
	}

	return groups;
}
