#include "lexer.hh"
#include "parser.hh"

using cchell::parser::ast_node;


auto
ast_node::set_type(ast_type type) noexcept -> ast_node &
{
    this->type = type;
    return *this;
}


auto
ast_node::set_source(source_location source) noexcept -> ast_node &
{
    this->source = source;
    return *this;
}


auto
ast_node::set_parent(ast_node *parent) noexcept -> ast_node &
{
    this->parent = parent;
    return *this;
}


auto
ast_node::set_data(std::string_view data) noexcept -> ast_node &
{
    this->data = data;
    return *this;
}


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
