#include "io/MeshLoader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string to_lower_copy(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch)
		{
			return static_cast<char>(std::tolower(ch));
		});
	return value;
}

int parse_obj_vertex_index(const std::string& token, std::size_t vertex_count)
{
	const std::size_t slash_pos = token.find('/');
	const std::string index_token = slash_pos == std::string::npos ? token : token.substr(0, slash_pos);

	if (index_token.empty())
	{
		throw std::runtime_error("OBJ face token is missing vertex index.");
	}

	const int raw_index = std::stoi(index_token);
	if (raw_index == 0)
	{
		throw std::runtime_error("OBJ uses 1-based indices, zero index is invalid.");
	}

	if (raw_index > 0)
	{
		const int zero_based = raw_index - 1;
		if (zero_based < 0 || zero_based >= static_cast<int>(vertex_count))
		{
			throw std::runtime_error("OBJ face index is out of bounds.");
		}
		return zero_based;
	}

	const int reverse_index = static_cast<int>(vertex_count) + raw_index;
	if (reverse_index < 0 || reverse_index >= static_cast<int>(vertex_count))
	{
		throw std::runtime_error("OBJ negative face index is out of bounds.");
	}
	return reverse_index;
}

Cloth load_obj(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open mesh file: " + path.string());
	}

	std::vector<glm::vec3> positions;
	std::vector<unsigned int> indices;
	std::string line;
	std::size_t line_number = 0;

	while (std::getline(file, line))
	{
		++line_number;

		std::istringstream line_stream(line);
		std::string prefix;
		line_stream >> prefix;

		if (prefix.empty() || prefix[0] == '#')
		{
			continue;
		}

		if (prefix == "v")
		{
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			if (!(line_stream >> x >> y >> z))
			{
				throw std::runtime_error("Invalid vertex definition at line " + std::to_string(line_number));
			}
			positions.emplace_back(x, y, z);
			continue;
		}

		if (prefix == "f")
		{
			std::vector<unsigned int> face_indices;
			std::string token;
			while (line_stream >> token)
			{
				const int parsed = parse_obj_vertex_index(token, positions.size());
				face_indices.push_back(static_cast<unsigned int>(parsed));
			}

			if (face_indices.size() < 3)
			{
				throw std::runtime_error("Face has fewer than 3 vertices at line " + std::to_string(line_number));
			}

			for (std::size_t i = 1; i + 1 < face_indices.size(); ++i)
			{
				indices.push_back(face_indices[0]);
				indices.push_back(face_indices[i]);
				indices.push_back(face_indices[i + 1]);
			}
			continue;
		}
	}

	if (positions.empty())
	{
		throw std::runtime_error("Mesh has no vertex positions: " + path.string());
	}

	if (indices.empty())
	{
		throw std::runtime_error("Mesh has no triangle faces: " + path.string());
	}

	return Cloth(std::move(positions), std::move(indices));
}
}

namespace io
{
Cloth load_cloth(const std::filesystem::path& path)
{
	const std::string extension = to_lower_copy(path.extension().string());
	if (extension == ".obj")
	{
		return load_obj(path);
	}

	throw std::runtime_error("Unsupported mesh format: " + path.extension().string());
}
}
