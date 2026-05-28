#pragma once
#include "FeatureExtractor.h"
#include "ofMain.h"

struct SimilarityResult {
	int index;
	float distance;
};

class SimilarityEngine {
public:
	// calcula distancia entre dois items
	float distance(const Features & a, const Features & b);

	// encontra os N items mais similares ao item no indice dado
	vector<SimilarityResult> findSimilar(int index, vector<Features> & allFeatures, int topN = 3);

	// agrupa items por similaridade — retorna indices agrupados
	vector<vector<int>> groupBySimilarity(vector<Features> & allFeatures, float threshold = 0.3f);

	// pesos de cada feature na distancia
	float weightLuminance = 1.0f;
	float weightVariance = 0.5f;
	float weightEdges = 2.0f;
	float weightKeypoints = 1.0f;
	float weightTexture = 1.0f;
};
