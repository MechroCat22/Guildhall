/************************************************************************/
/* File: VoxelAnimationSet.cpp
/* Author: Andrew Chase
/* Date: October 2nd, 2018
/* Description: Implementation of the VoxelAnimationSet class
/************************************************************************/
#include "Game/Animation/VoxelAnimationSet.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Utility/StringUtils.hpp"
#include "Engine/Core/Utility/ErrorWarningAssert.hpp"

// Global list of animation sets in the game
std::map<std::string, const VoxelAnimationSet*> VoxelAnimationSet::s_animationSets;


//-----------------------------------------------------------------------------------------------
// Constructor from an XML element
//
VoxelAnimationSet::VoxelAnimationSet(const XMLElement& setElement)
{
	std::string setName = ParseXmlAttribute(setElement, "name");
	if (setName.size() == 0)
	{
		ERROR_AND_DIE("AnimationSet::AnimationSet() parsed file with no name specified in root element");
	}

	m_name = setName;

	// Read in each alias element and parse the data
	const XMLElement* aliasElement = setElement.FirstChildElement();

	while (aliasElement != nullptr)
	{
		std::string alias = ParseXmlAttribute(*aliasElement, "alias");

		if (alias.size() == 0)
		{
			ERROR_AND_DIE(Stringf("AnimationSet::AnimationSet() found alias element with no alias specified - set was %s", setName.c_str()));
		}

		const XMLElement* animationElement = aliasElement->FirstChildElement();

		// Get each animation name for this alias
		while (animationElement != nullptr)
		{
			std::string animationName = ParseXmlAttribute(*animationElement, "name");

			if (animationName.size() == 0)
			{
				ERROR_AND_DIE(Stringf("AnimationSet::AnimationSet() found animation element with no name specified - set was %s", setName.c_str()));
			}

			m_translations[alias].push_back(animationName);

			animationElement = animationElement->NextSiblingElement();
		}

		aliasElement = aliasElement->NextSiblingElement();
	}
}


//-----------------------------------------------------------------------------------------------
// Returns the name of this set
//
std::string VoxelAnimationSet::GetName() const
{
	return m_name;
}


//-----------------------------------------------------------------------------------------------
// Picks a given translation (animation name) associated with the given alias
// Returns true if a translation was found, false otherwise
//
bool VoxelAnimationSet::TranslateAlias(const std::string& alias, std::string& out_translation) const
{
	bool aliasExists = m_translations.find(alias) != m_translations.end();

	if (aliasExists)
	{
		const std::vector<std::string>& possibleNames = m_translations.at(alias);
		int randomIndex = GetRandomIntLessThan((int)possibleNames.size());

		out_translation = possibleNames[randomIndex];
	}

	return aliasExists;
}


//-----------------------------------------------------------------------------------------------
// Loads an animation set from the XML file given by filename
//
const VoxelAnimationSet* VoxelAnimationSet::LoadSet(const std::string& filename)
{
	// Load the document
	XMLDocument document;
	XMLError error = document.LoadFile(filename.c_str());

	if (error != tinyxml2::XML_SUCCESS)
	{
		ERROR_AND_DIE(Stringf("AnimationSet::LoadSetFromFile() couldn't open file %s", filename.c_str()));
		return nullptr;
	}

	const XMLElement* rootElement = document.RootElement();

	VoxelAnimationSet* newSet = new VoxelAnimationSet(*rootElement);
	s_animationSets[newSet->GetName()] = newSet;

	return newSet;
}


//-----------------------------------------------------------------------------------------------
// Returns the animation set given by setName, nullptr if it doesn't exist
//
const VoxelAnimationSet* VoxelAnimationSet::GetAnimationSet(const std::string& setName)
{
	bool setExists = s_animationSets.find(setName) != s_animationSets.end();

	if (setExists)
	{
		return s_animationSets.at(setName);
	}

	return nullptr;
}
