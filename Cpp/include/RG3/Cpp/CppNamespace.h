#pragma once

#include <string>
#include <vector>


namespace rg3::cpp
{
    class CppNamespace
    {
    public:
        CppNamespace();
        CppNamespace(std::string aNamespace);

        [[nodiscard]] const std::string& operator[](size_t i) const;
        [[nodiscard]] std::string& operator[](size_t i);

		CppNamespace& operator=(const CppNamespace&);
		CppNamespace& operator/(const std::string&);
		bool operator==(const CppNamespace& other) const;
		bool operator!=(const CppNamespace& other) const;

		operator std::string() const { return m_sNamespace; }

		void prepend(const std::string& part);

		const std::string& asString() const { return m_sNamespace; }

		bool isEmpty() const { return m_vNamespace.empty(); }

    private:
        void parseNamespace();

    private:
        std::string m_sNamespace;
        std::vector<std::string> m_vNamespace;
    };
}