from Cython.CodeWriter import CodeWriter as BaseWriter


class CodeWriter(BaseWriter):

    def visit_CFuncDefNode(self, node):
        """Handler for CFuncDefNode types"""
        if 'inline' in node.modifiers:
            return
        if node.overridable:
            self.startline(u'cpdef ')
        else:
            self.startline(u'cdef ')
        if node.visibility != 'private':
            self.put(node.visibility)
            self.put(u' ')
        if node.api:
            self.put(u'api ')
        self.visit(node.declarator)
        self.indent()
        self.visit(node.body)
        self.dedent()

    def visit_CFuncDeclaratorNode(self, node):
        """Handler for CFuncDeclaratorNode types"""
        # TODO: except, gil, etc.
        self.visit(node.base)
        self.put(u'(')
        self.comma_separated_list(node.args)
        self.endline(u'):')

    def visit_IndexNode(self, node):
        """Handler for visit_IndexNode types"""
        self.put("[")