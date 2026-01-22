#include "lexer.hh"
#include "parser.hh"
#include "parser/impl.hh"


auto
cchell::parser::parse(const std::vector<lexer::token> &tokens)
    -> std::unique_ptr<ast_node>
{
    auto root { std::make_unique<ast_node>(
        ast_node {}.set_type(ast_type::statement)) };

    bool found_command { false };

    for (const lexer::token &token : tokens)
        if (!found_command)
        {
            if (impl::assignment(token, *root)) continue;
            if (impl::command(token, *root))
            {
                found_command = true;
                continue;
            }
        }
        else
        {
            if (impl::string(token, *root)) continue;
            if (impl::option(token, *root)) continue;
        }

    return root;
}


auto
cchell::parser::impl::verifier::operator()(ast_node &nodes) const
    -> std::optional<diagnostic>
{
    for (auto &node : nodes.child)
    {
        if (!node.child.empty())
            if (auto diag { verify(node) }) return diag;

        if (node.type == ast_type::command)
            if (auto diag { impl::verify_command(node) }) return diag;
    }

    return std::nullopt;
}
