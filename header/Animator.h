#pragma once

class Animator
{
private:
	
	std::vector<mat4>m_FinalBoneMatrices;
	Animation* m_CurrentAnimation;
	float m_CurrentTime;
	float m_DeltaTime;

public:
	Animator(Animation* animation)
	{
		m_CurrentTime = 0.0f;
		m_CurrentAnimation = animation;

		m_FinalBoneMatrices.reserve(100);

		for (int i = 0; i < 100; i++)
		{
			m_FinalBoneMatrices.push_back(mat4::identity());
		}
	}

	void UpdateAnimation(float dt)
	{
		m_DeltaTime = dt;
		if (m_CurrentAnimation)
		{
			m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * m_DeltaTime;
			m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
			CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), mat4::identity());
		}
	}

	void PlayAnimation(Animation* pAnimation)
	{
		m_CurrentAnimation = pAnimation;
		m_CurrentTime = 0.0f;
	}

	void CalculateBoneTransform(const AssimpNodeData* node, mat4 parentTransform)
	{
		std::string nodeName = node->name;
		mat4 nodeTransform = node->transformation;

		Bone* bone = m_CurrentAnimation->FindBone(nodeName);

		if (bone)
		{
			bone->Update(m_CurrentTime);
			nodeTransform = bone->GetLocalTransform();
		}

		mat4 globalTransformation = parentTransform * nodeTransform;

		std::map<std::string, BoneInfo> boneInfoMap = m_CurrentAnimation->GetBoneIDMap();

		if (boneInfoMap.find(nodeName) != boneInfoMap.end())
		{
			int index = boneInfoMap[nodeName].id;
			mat4 offset = boneInfoMap[nodeName].offset;
			m_FinalBoneMatrices[index] = globalTransformation * offset;
		}

		for (int i = 0; i < node->childrenCount; i++)
		{
			CalculateBoneTransform(&node->children[i], globalTransformation);
		}
	}

	std::vector<mat4> GetFinalBoneMatrices()
	{
		return m_FinalBoneMatrices;
	}
};
