#include "parser.h"

std::vector<std::string> parse(const std::string &input)

{
  std::vector<std::string> filteredToken;
  std::string curr_token = "";
  bool in_single_quotes = false;
  bool in_double_quotes = false;
  for (size_t i = 0; i < input.size(); i++)
  {
    if (input[i] == '\\' && in_double_quotes)
    {
      if (input[i + 1] == '\\' || input[i + 1] == '"')
      {
        curr_token += input[i + 1];
        i++;
      }
      else
      {
        curr_token += input[i];
      }
    }
    else if (input[i] == '\\' && !in_double_quotes && !in_single_quotes)
    {
      if (i + 1 < input.size())
      {
        curr_token += input[i + 1];
        i++;
      }
    }
    else if (input[i] == '\"' && in_double_quotes)
    {
      in_double_quotes = false;
    }
    else if (input[i] == '\'' && in_single_quotes)
    {
      in_single_quotes = false;
    }
    else if (input[i] == '\'' && !in_double_quotes)
    {
      in_single_quotes = true;
    }
    else if (input[i] == '\"' && !in_single_quotes)
    {
      in_double_quotes = true;
    }
    else if (std::isspace(input[i]) && !in_single_quotes && !in_double_quotes)
    {
      if (!curr_token.empty())
      {
        filteredToken.push_back(curr_token);
        curr_token.clear();
      }
    }
    else
    {
      curr_token += input[i];
    }
  }
  if (!curr_token.empty())
  {
    filteredToken.push_back(curr_token);
  }
  return filteredToken;
}