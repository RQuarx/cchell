#include "evaluator.hh"
#include "parser.hh"

using cchell::evaluator::process;
using namespace cchell;


namespace
{

}



process::process(std::unique_ptr<parser::ast_node> &&nodes)
    : m_ast { std::move(nodes) }
{
}


int
process::run()
{

}

void
process::wait()
{

}
