#pragma once

struct KeyPosition
{
	vec3 position;
	float timeStamp;
};

struct KeyRotation
{
	quaternion orientation;
	float timeStamp;
};

struct KeyScale
{
	vec3 scale;
	float timeStamp;
};

static inline mat4 ConvertMatrixToVmathFormat(const aiMatrix4x4& from)
{
	mat4 to = mat4::identity();

	to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
	to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
	to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
	to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;

	return to;
}

static inline vec3 GetVmathVec(const aiVector3D& vec)
{
	return vec3(vec.x, vec.y, vec.z);
}

static inline quaternion GetVmathQuat(const aiQuaternion& pOrientation)
{
	return quaternion(pOrientation.x, pOrientation.y, pOrientation.z, pOrientation.w);
}

class Bone
{
public:
	Bone(const std::string& name, int ID, const aiNodeAnim* channel)
	{
		m_name = name;
		m_ID = ID;
		m_localTransform = mat4::identity();

		m_NumPosition = channel->mNumPositionKeys;

		for (size_t positionIndex = 0; positionIndex < m_NumPosition; positionIndex++)
		{
			aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
			float timeStamp = channel->mPositionKeys[positionIndex].mTime;

			KeyPosition data;
			data.position = GetVmathVec(aiPosition);
			data.timeStamp = timeStamp;
			m_Positions.push_back(data);
		}

		m_NumRotation = channel->mNumRotationKeys;

		for (size_t rotationIndex = 0; rotationIndex < m_NumRotation; rotationIndex++)
		{
			aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
			float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
			KeyRotation data;
			data.orientation = GetVmathQuat(aiOrientation);
			data.timeStamp = timeStamp;
			m_Rotations.push_back(data);
		}

		m_NumScaling = channel->mNumScalingKeys;

		for (size_t index = 0; index < m_NumScaling; index++)
		{
			aiVector3D aiScale = channel->mScalingKeys[index].mValue;
			float timeStamp = channel->mScalingKeys[index].mTime;

			KeyScale data;
			data.scale = GetVmathVec(aiScale);
			data.timeStamp = timeStamp;
			m_Scales.push_back(data);
		}
	}

	void Update(float animationTime)
	{
		mat4 translation = InterpolatePosition(animationTime);
		mat4 rotation = InterPolateRotation(animationTime);
		mat4 scale = InterPolateScaling(animationTime);

		m_localTransform = translation * rotation * scale;

	}

	mat4 GetLocalTransform() { return m_localTransform; }
	std::string GetBoneName() { return m_name; }
	int GetBoneID() { return m_ID; }

	int GetPositionIndex(float animationTime)
	{
		for (size_t i = 0; i < m_NumPosition - 1; i++)
		{
			if (animationTime < m_Positions[i + 1].timeStamp)
				return i;
		}
		return 0;
	}

	int GetRotationIndex(float animationTime)
	{
		for (size_t i = 0; i < m_NumRotation - 1; i++)
		{
			if (animationTime < m_Rotations[i + 1].timeStamp)
				return i;
		}
		return 0;
	}

	int GetScaleIndex(float animationTime)
	{
		for (size_t i = 0; i < m_NumScaling - 1; i++)
		{
			if (animationTime < m_Scales[i + 1].timeStamp)
				return i;
		}
		return 0;
	}

private:

	float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
	{
		float scaleFactor = 0.0f;
		float midWayLength = animationTime - lastTimeStamp;
		float frameDiff = nextTimeStamp - lastTimeStamp;
		scaleFactor = midWayLength / frameDiff;
		return scaleFactor;
	}

	mat4 InterpolatePosition(float animationTime)
	{
		if (m_NumPosition == 1)
			return translate(m_Positions[0].position);
		int p0Index = GetPositionIndex(animationTime);
		int p1Index = p0Index + 1;
		float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp, m_Positions[p1Index].timeStamp, animationTime);
		vec3 finalPosition = mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
		return translate(finalPosition);
	}

	mat4 InterPolateRotation(float animationTime)
	{
		if (m_NumRotation == 1)
		{
			quaternion rotation = normalize(m_Rotations[0].orientation);
			return rotation.asMatrix();
		}

		int p0Index = GetRotationIndex(animationTime);
		int p1Index = p0Index + 1;

		float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);

		quaternion finalRotation = slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation,scaleFactor);
		
		finalRotation = normalize(finalRotation);
		return finalRotation.asMatrix();
	}

	mat4 InterPolateScaling(float animationTime)
	{
		if (m_NumScaling == 1)
			return scale(m_Scales[0].scale);

		int p0Index = GetScaleIndex(animationTime);
		int p1Index = p0Index + 1;

		float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);

		vec3 finalScale = mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
		return scale(finalScale);
	}

	std::vector<KeyPosition> m_Positions;
	std::vector<KeyRotation> m_Rotations;
	std::vector<KeyScale> m_Scales;

	int m_NumPosition;
	int m_NumRotation;
	int m_NumScaling;

	mat4 m_localTransform;
	std::string m_name;
	int m_ID;
};
