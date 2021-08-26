# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
from __future__ import annotations

import ast

from ..optimizer import AstOptimizer


class AstOptimizer38(AstOptimizer):
    def visitNamedExpr(self, node: ast.NamedExpr) -> ast.NamedExpr:
        return node
